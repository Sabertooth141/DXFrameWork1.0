#include "SkeletalAnimatorComponent.h"

#include <algorithm>

#include "AnimationClip.h"
#include "GameContext.h"
#include "SkeletalAnimationSystem.h"

namespace
{
	// VectorKeyInfo && QuatKeyInfo
	template <typename KeyInfo>
	unsigned AdvanceKeyCursor(const std::vector<KeyInfo>& keys, float ticks, unsigned& cursor)
	{
		// since cursor will advance by 1 last starting key = size - 2
		const unsigned lastValidSegmentStart = static_cast<unsigned>(keys.size() - 2);

		if (cursor > lastValidSegmentStart || keys[cursor].time > ticks)
		{
			cursor = 0;
		}

		while (cursor < lastValidSegmentStart && keys[cursor + 1].time <= ticks)
		{
			cursor++;
		}
		return cursor;
	}

	// VectorKeyInfo && QuatKeyInfo
	template <typename KeyInfo>
	float SegmentProgress(const KeyInfo& startKey, const KeyInfo& finishKey, float ticks)
	{
		const float span = finishKey.time - startKey.time;
		if (span <= 0.f)
		{
			return 0.f;
		}

		return std::clamp((ticks - startKey.time) / span, 0.f, 1.f);
	}

	DirectX::XMVECTOR SampleVectorKeyInfo(const std::vector<VectorKeyInfo>& keys, float ticks, unsigned& cursor,
	                                      DirectX::FXMVECTOR fallback)
	{
		if (keys.empty())
		{
			return fallback;
		}

		if (keys.size() == 1)
		{
			return DirectX::XMLoadFloat3(&keys[0].value);
		}

		const unsigned keyIndex = AdvanceKeyCursor(keys, ticks, cursor);
		const float progress = SegmentProgress(keys[keyIndex], keys[keyIndex + 1], ticks);

		return DirectX::XMVectorLerp(DirectX::XMLoadFloat3(&keys[keyIndex].value),
		                             DirectX::XMLoadFloat3(&keys[keyIndex + 1].value), progress);
	}

	DirectX::XMVECTOR SampleRotation(const std::vector<QuatKeyInfo>& keys, float ticks, unsigned& cursor,
	                                 DirectX::FXMVECTOR fallback)
	{
		if (keys.empty())
		{
			return fallback;
		}

		if (keys.size() == 1)
		{
			return DirectX::XMLoadFloat4(&keys[0].value);
		}

		const unsigned keyIndex = AdvanceKeyCursor(keys, ticks, cursor);
		const float progress = SegmentProgress(keys[keyIndex], keys[keyIndex + 1], ticks);

		const DirectX::XMVECTOR startQuat = DirectX::XMLoadFloat4(&keys[keyIndex].value);
		DirectX::XMVECTOR finishQuat = DirectX::XMLoadFloat4(&keys[keyIndex + 1].value);

		// take the shortest rotation of q and -q from start to finish
		if (DirectX::XMVectorGetX(DirectX::XMQuaternionDot(startQuat, finishQuat)) < 0.f)
		{
			finishQuat = DirectX::XMVectorNegate(finishQuat);
		}

		return DirectX::XMQuaternionNormalize(DirectX::XMQuaternionSlerp(startQuat, finishQuat, progress));
	}
}

SkeletalAnimatorComponent::SkeletalAnimatorComponent(Renderer& renderer, std::shared_ptr<const Skeleton> inSkeleton) :
	skeleton(std::move(inSkeleton)),
	paletteCBuffer(renderer, palette, 1) // b1
{
	const size_t nodeCount = skeleton->nodes.size();
	localPose.resize(nodeCount);
	globalPose.resize(nodeCount);

	// init as bind pose
	for (size_t i = 0; i < nodeCount; i++)
	{
		localPose[i] = skeleton->nodes[i].bindLocal;
	}

	for (auto& boneMatrix : palette.bones)
	{
		DirectX::XMStoreFloat4x4(&boneMatrix, DirectX::XMMatrixIdentity());
	}

	ComputeGlobals();
	ComputePalette();

#ifdef _DEBUG
	// in bind pose every final matrix should be identity
	float maxError = 0.f;
	for (size_t b = 0; b < skeleton->bones.size(); b++)
	{
		for (int r = 0; r < 4; r++)
		{
			for (int c = 0; c < 4; c++)
			{
				const float expected = (r == c) ? 1.f : 0.f;
				maxError = (std::max)(maxError, std::fabs(palette.bones[b].m[r][c] - expected));
			}
		}
	}
	char msg[128];
	sprintf_s(msg, "[SkeletalAnimator] bind palette: max |final - I| = %g\n", maxError);
	OutputDebugStringA(msg);
#endif
}

void SkeletalAnimatorComponent::OnAttach(GameContext& context)
{
	IComponent::OnAttach(context);
	context.skeletalAnimationSys.Register(this);
}

void SkeletalAnimatorComponent::Update(float deltaTime)
{
	if (!currClip)
	{
		return;
	}

	AdvanceTime(deltaTime);
	SamplePose();
	ComputeGlobals();
	ComputePalette();
}

void SkeletalAnimatorComponent::SetCurrClip(std::shared_ptr<const AnimationClip> inClip, bool inLoop)
{
	assert((!inClip || inClip->builtFor == skeleton.get()) && "clip was mapped against a different skeleton");

	currClip = std::move(inClip);
	isLoop = inLoop;
	timeInSec = 0.f;
	isFinished = false;
	cursors.assign(currClip ? currClip->channels.size() : 0, KeyCursor{});
}

void SkeletalAnimatorComponent::AddClip(std::shared_ptr<const AnimationClip> inClip, bool inLoop)
{
	if (!inClip)
	{
		return;
	}

	const std::string name = inClip->name;
	clipEntries[name] = {std::move(inClip), inLoop};
}

bool SkeletalAnimatorComponent::PlayClip(const std::string& name, float fadeSec)
{
	auto it = clipEntries.find(name);
	if (it == clipEntries.end())
	{
		OutputDebugStringA(("[SkeletalAnimator] no clip named '" + name + "\'\n").c_str());
		return false;
	}

	if (fadeSec < 0.f)
	{
		fadeSec = fadeTable ? fadeTable->GetFadeTime(GetCurrClipName(), name) : 0.f;
	}

#ifndef NDEBUG
	char msg[256];
	sprintf_s(msg, "[SkeletalAnimator] %s -> %s (fade %.2fs, snapped until crossfade)\n",
		GetCurrClipName().c_str(), name.c_str(), fadeSec);
	OutputDebugStringA(msg);
#endif

	SetCurrClip(it->second.clip, it->second.isLoop);
	return true;
}

const std::string& SkeletalAnimatorComponent::GetCurrClipName() const
{
	static const std::string none;
	return currClip ? currClip->name : none;
}

std::vector<std::string> SkeletalAnimatorComponent::GetClipNames() const
{
	std::vector<std::string> names;
	names.reserve(clipEntries.size());
	for (const auto& [name, entry] : clipEntries)
	{
		names.push_back(name);
	}

	std::ranges::sort(names);
	return names;
}

void SkeletalAnimatorComponent::AdvanceTime(float deltaTime)
{
	const float length = currClip->DurationInSeconds();
	if (length <= 0.f)
	{
		timeInSec = 0.f;
		isFinished = !isLoop;
		return;
	}

	timeInSec += deltaTime;
	if (isLoop)
	{
		timeInSec = std::fmod(timeInSec, length);
	}
	else if (timeInSec >= length)
	{
		timeInSec = length;
		isFinished = true;
	}
}

void SkeletalAnimatorComponent::SamplePose()
{
	// nodes without a channel stay in bind pose
	for (size_t i = 0; i < skeleton->nodes.size(); i++)
	{
		localPose[i] = skeleton->nodes[i].bindLocal;
	}

	const float ticks = timeInSec * currClip->ticksPerSecond;

	for (size_t channelIndex = 0; channelIndex < currClip->channels.size(); channelIndex++)
	{
		const int nodeIndex = currClip->channelToNode[channelIndex];

		if (nodeIndex < 0)
		{
			continue;
		}

		const AnimationChannel& channel = currClip->channels[channelIndex];
		const SkeletonNode& skeletonNode = skeleton->nodes[nodeIndex];
		KeyCursor& cursor = cursors[channelIndex];

		const DirectX::XMVECTOR translation = SampleVectorKeyInfo(channel.positions, ticks, cursor.position,
		                                                          DirectX::XMLoadFloat3(&skeletonNode.bindTranslation));
		const DirectX::XMVECTOR rotation = SampleRotation(channel.rotations, ticks, cursor.rotation,
		                                                  DirectX::XMLoadFloat4(&skeletonNode.bindRotation));
		const DirectX::XMVECTOR scale = SampleVectorKeyInfo(channel.scales, ticks, cursor.scale,
		                                                    DirectX::XMLoadFloat3(&skeletonNode.bindScale));

		// row vectors: SRT
		const DirectX::XMMATRIX local = DirectX::XMMatrixScalingFromVector(scale) *
			DirectX::XMMatrixRotationQuaternion(rotation) *
			DirectX::XMMatrixTranslationFromVector(translation);
		DirectX::XMStoreFloat4x4(&localPose[nodeIndex], local);
	}
}

void SkeletalAnimatorComponent::BindPalette(Renderer& renderer)
{
	paletteCBuffer.Update(renderer, palette);
	paletteCBuffer.Bind(renderer);
}

void SkeletalAnimatorComponent::ComputeGlobals()
{
	// parents first
	for (size_t i = 0; i < skeleton->nodes.size(); i++)
	{
		const DirectX::XMMATRIX local = DirectX::XMLoadFloat4x4(&localPose[i]); // load to matrix from 4x4
		const int parentIndex = skeleton->nodes[i].parent;

		const DirectX::XMMATRIX global = (parentIndex < 0)
			                                 ? local
			                                 : local * DirectX::XMLoadFloat4x4(&globalPose[parentIndex]);
		DirectX::XMStoreFloat4x4(&globalPose[i], global);
	}
}

void SkeletalAnimatorComponent::ComputePalette()
{
	const DirectX::XMMATRIX meshSpaceInverse = DirectX::XMLoadFloat4x4(&skeleton->meshSpaceInverse);

	for (size_t boneIndex = 0; boneIndex < skeleton->bones.size(); boneIndex++)
	{
		const SkeletonBone& bone = skeleton->bones[boneIndex];
		const DirectX::XMMATRIX finalMatrix =
			DirectX::XMLoadFloat4x4(&bone.offset) *
			DirectX::XMLoadFloat4x4(&globalPose[bone.node]) *
			meshSpaceInverse;

		DirectX::XMStoreFloat4x4(&palette.bones[boneIndex], DirectX::XMMatrixTranspose(finalMatrix));
	}
}
