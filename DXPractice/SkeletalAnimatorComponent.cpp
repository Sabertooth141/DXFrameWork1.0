#include "SkeletalAnimatorComponent.h"

#include <algorithm>

#include "AnimationClip.h"
#include "GameContext.h"
#include "GameObject.h"
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

	DirectX::XMVECTOR BlendRotation(DirectX::FXMVECTOR startRot, DirectX::FXMVECTOR finishRot, float weight)
	{
		DirectX::XMVECTOR target = finishRot;

		// take the shortest rotation of q and -q from start to finish
		if (DirectX::XMVectorGetX(DirectX::XMQuaternionDot(startRot, finishRot)) < 0.f)
		{
			target = DirectX::XMVectorNegate(target);
		}

		return DirectX::XMQuaternionNormalize(DirectX::XMQuaternionSlerp(startRot, target, weight));
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

		return BlendRotation(startQuat, finishQuat, progress);
	}

	LocalTRS BlendTRS(const LocalTRS& startTRS, const LocalTRS& finishTRS, float weight)
	{
		LocalTRS out;
		DirectX::XMStoreFloat3(&out.translation,
		                       DirectX::XMVectorLerp(DirectX::XMLoadFloat3(&startTRS.translation),
		                                             DirectX::XMLoadFloat3(&finishTRS.translation), weight));
		DirectX::XMStoreFloat4(&out.rotation,
		                       BlendRotation(DirectX::XMLoadFloat4(&startTRS.rotation),
		                                     DirectX::XMLoadFloat4(&finishTRS.rotation), weight));
		DirectX::XMStoreFloat3(&out.scale,
		                       DirectX::XMVectorLerp(DirectX::XMLoadFloat3(&startTRS.scale),
		                                             DirectX::XMLoadFloat3(&finishTRS.scale), weight));

		return out;
	}
}

SkeletalAnimatorComponent::SkeletalAnimatorComponent(Renderer& renderer, std::shared_ptr<const Skeleton> inSkeleton) :
	skeleton(std::move(inSkeleton)),
	paletteCBuffer(renderer, palette, 1) // b1
{
	const size_t nodeCount = skeleton->nodes.size();
	currPoseTRSes.resize(nodeCount);
	sampledPoseTRSes.resize(nodeCount);
	localMatrices.resize(nodeCount);
	globalPose.resize(nodeCount);

	// init as bind pose
	for (size_t i = 0; i < nodeCount; i++)
	{
		currPoseTRSes[i] = skeleton->nodes[i].bindTRS;
	}

	for (auto& boneMatrix : palette.bones)
	{
		DirectX::XMStoreFloat4x4(&boneMatrix, DirectX::XMMatrixIdentity());
	}

	BuildLocalMatrices();
	ComputeGlobals();
	ComputePalette();

#ifndef NDEBUG
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
	if (playback.clip)
	{
		Tick(deltaTime * timeScale);
	}

	firedEvents.swap(pendingEvents);
	pendingEvents.clear();
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
	if (fade.isActive)
	{
		sprintf_s(msg, "[SkeletalAnimator] %s -> %s (fade %.2fs, interrupting fade at w=%.2f)\n",
		          GetCurrClipName().c_str(), name.c_str(), fadeSec, fade.elapsedSec / fade.durationSec);
	}
	else
	{
		sprintf_s(msg, "[SkeletalAnimator] %s -> %s (fade %.2fs)\n",
		          GetCurrClipName().c_str(), name.c_str(), fadeSec);
	}
	OutputDebugStringA(msg);
#endif

	// report interrupt if not exited properly
	if (playback.clip && !playback.isFinished)
	{
		pendingEvents.push_back({FiredEventType::INTERRUPTED, playback.clip->name, {}, playback.timeInSec});
	}

	StartFade(fadeSec);
	StartPlayback(it->second);
	return true;
}

void SkeletalAnimatorComponent::AddEvent(const std::string& clipName, float timeSec, const std::string& eventName, const std::string& param)
{
	auto it = clipEntries.find(clipName);
	if (it == clipEntries.end())
	{
		OutputDebugStringA(("[SkeletalAnimator] AddEvent: no clip named '" + clipName + "'\n").c_str());
		assert(false && "AddEvent before AddClip, or clip name typo (case sensitive)");
		return;
	}

	ClipEntry& entry = it->second;
	const float length = entry.clip->DurationInSeconds();
	assert(timeSec >= 0.f && timeSec <= length && "event time outside the clip");
	timeSec = std::clamp(timeSec, 0.f, length);

	// find position by time
	auto pointer = std::ranges::upper_bound(entry.events, timeSec, {}, &AnimationEvent::timeSec);
	entry.events.insert(pointer, AnimationEvent{timeSec, eventName, param});
}

float SkeletalAnimatorComponent::GetCurrClipLength() const
{
	if (playback.clip)
	{
		return playback.clip->DurationInSeconds();
	}
	else
	{
		return 0.f;
	}
}

void SkeletalAnimatorComponent::Scrub(float deltaSec)
{
	if (playback.clip)
	{
		Tick(deltaSec);
	}
}

void SkeletalAnimatorComponent::Tick(float clipDeltaSec)
{
	const float prevTimeSec = playback.timeInSec;
	AdvanceTime(playback, clipDeltaSec);
	CollectEvents(prevTimeSec, clipDeltaSec);

	if (fade.isActive)
	{
		SampleClip(playback, sampledPoseTRSes);
		UpdateFade(clipDeltaSec);
	}
	else
	{
		SampleClip(playback, currPoseTRSes);
	}

	BuildLocalMatrices();
	ComputeGlobals();
	ComputePalette();
}

void SkeletalAnimatorComponent::CollectEvents(float prevTimeSec, float deltaSec)
{
	const bool includeStart = playback.isFreshStart;
	playback.isFreshStart = false;

	// no events/ backwards doesnt fire event
	if (!playback.events || playback.events->empty() || deltaSec < 0.f)
	{
		return;
	}

	const float currTimeSec = playback.timeInSec;
	if (!playback.isLoop)
	{
		ClampToRange(prevTimeSec, currTimeSec, includeStart);
		return;
	}

	const float length = playback.clip->DurationInSeconds();
	if (deltaSec >= length)
	{
		ClampToRange(0.f, length, true);
		return;
	}

	if (currTimeSec >= prevTimeSec)
	{
		ClampToRange(prevTimeSec, currTimeSec, includeStart);
	}
	else
	{
		ClampToRange(prevTimeSec, length, includeStart);
		ClampToRange(0.f, currTimeSec, true);
	}
}

void SkeletalAnimatorComponent::ClampToRange(float startSec, float finishSec, bool isIncludingStartSec)
{
	for (const AnimationEvent& event : *playback.events)
	{
		if (event.timeSec > finishSec)
		{
			break;
		}

		bool isAfterStart;
		if (isIncludingStartSec)
		{
			isAfterStart = event.timeSec >= startSec;
		}
		else
		{
			isAfterStart = event.timeSec > startSec;
		}

		if (isAfterStart)
		{
			pendingEvents.push_back({FiredEventType::MARKER, playback.clip->name, event.eventName, event.timeSec, event.param});
		}
	}
}

const std::string& SkeletalAnimatorComponent::GetCurrClipName() const
{
	static const std::string none;
	return playback.clip ? playback.clip->name : none;
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

int SkeletalAnimatorComponent::FindNodeIndex(const std::string& name) const
{
	const auto it = skeleton->nodeIndexByName.find(name);
	if (it == skeleton->nodeIndexByName.end())
	{
		OutputDebugStringA(("[SkeletalAnimator] FindNodeIndex: no node named '" + name + "'\n").c_str());
		return -1;
	}

	return it->second;
}

DirectX::XMMATRIX SkeletalAnimatorComponent::GetNodeWorldMatrix(int nodeIndex) const
{
	assert(nodeIndex >= 0 && nodeIndex < static_cast<int>(globalPose.size()));

	return DirectX::XMLoadFloat4x4(&globalPose[nodeIndex]) *
		DirectX::XMLoadFloat4x4(&skeleton->meshSpaceInverse) *
		owner->GetTransform()->GetMatrix();
}

DirectX::XMMATRIX SkeletalAnimatorComponent::GetBoneWorldMatrix(const std::string& name) const
{
	const int nodeIndex = FindNodeIndex(name);
	if (nodeIndex >= 0)
	{
		return GetNodeWorldMatrix(nodeIndex);
	}
	else
	{
		return owner->GetTransform()->GetMatrix();
	}
}

void SkeletalAnimatorComponent::StartPlayback(const ClipEntry& entry)
{
	assert(entry.clip->builtFor == skeleton.get() && "clip was mapped agaisnt a different skelton");

	playback.clip = entry.clip;
	playback.events = &entry.events;
	playback.isLoop = entry.isLoop;
	playback.timeInSec = 0.f;
	playback.isFinished = false;
	playback.isFreshStart = true;
	playback.cursors.assign(playback.clip->channels.size(), KeyCursor{});
}

void SkeletalAnimatorComponent::StartFade(float fadeSec)
{
	// if first animation or fade = 0 -> snap
	if (fadeSec <= 0 || !playback.clip)
	{
		fade.isActive = false;
		return;
	}

	fade.isActive = true;
	fade.source = FadeSource::SNAPSHOT;
	fade.durationSec = fadeSec;
	fade.elapsedSec = 0.f;
	fade.fromPose = currPoseTRSes;
}

void SkeletalAnimatorComponent::AdvanceTime(ClipPlayback& clipPlayback, float deltaTime)
{
	const float length = clipPlayback.clip->DurationInSeconds();
	if (length <= 0.f)
	{
		clipPlayback.timeInSec = 0.f;
		clipPlayback.isFinished = !clipPlayback.isLoop;
		return;
	}

	clipPlayback.timeInSec += deltaTime;
	if (clipPlayback.isLoop)
	{
		clipPlayback.timeInSec = std::fmod(clipPlayback.timeInSec, length);
		if (clipPlayback.timeInSec < 0.f) // for negative deltas
		{
			clipPlayback.timeInSec += length;
		}
	}
	else
	{
		clipPlayback.timeInSec = std::clamp(clipPlayback.timeInSec, 0.f, length);
		clipPlayback.isFinished = clipPlayback.timeInSec >= length;
	}
}

void SkeletalAnimatorComponent::SampleClip(ClipPlayback& clipPlayback, std::vector<LocalTRS>& outPose)
{
	// nodes without a channel stay in bind pose
	for (size_t i = 0; i < skeleton->nodes.size(); i++)
	{
		outPose[i] = skeleton->nodes[i].bindTRS;
	}

	const AnimationClip& clip = *clipPlayback.clip;
	const float ticks = clipPlayback.timeInSec * clip.ticksPerSecond;

	for (size_t channelIndex = 0; channelIndex < clip.channels.size(); channelIndex++)
	{
		const int nodeIndex = clip.channelToNode[channelIndex];

		if (nodeIndex < 0)
		{
			continue;
		}

		const AnimationChannel& channel = clip.channels[channelIndex];
		KeyCursor& cursor = clipPlayback.cursors[channelIndex];
		LocalTRS& trs = outPose[nodeIndex];

		// trs holds the bind values here, they double as the fallback for empty key lists
		DirectX::XMStoreFloat3(&trs.translation,
		                       SampleVectorKeyInfo(channel.positions, ticks, cursor.position,
		                                           DirectX::XMLoadFloat3(&trs.translation)));
		DirectX::XMStoreFloat4(&trs.rotation,
		                       SampleRotation(channel.rotations, ticks, cursor.rotation,
		                                      DirectX::XMLoadFloat4(&trs.rotation)));
		DirectX::XMStoreFloat3(&trs.scale,
		                       SampleVectorKeyInfo(channel.scales, ticks, cursor.scale,
		                                           DirectX::XMLoadFloat3(&trs.scale)));
	}
}

void SkeletalAnimatorComponent::UpdateFade(float deltaTime)
{
	fade.elapsedSec += deltaTime;
	const float weight = std::clamp(fade.elapsedSec / fade.durationSec, 0.f, 1.f);

	switch (fade.source)
	{
	case FadeSource::SNAPSHOT:
		// capture frompose in startfade and never changes
		break;
	}

	for (size_t i = 0; i < currPoseTRSes.size(); i++)
	{
		currPoseTRSes[i] = BlendTRS(fade.fromPose[i], sampledPoseTRSes[i], weight);
	}

	if (weight >= 1.f)
	{
		fade.isActive = false;
	}
}

void SkeletalAnimatorComponent::BuildLocalMatrices()
{
	for (size_t i = 0; i < currPoseTRSes.size(); i++)
	{
		const LocalTRS& trs = currPoseTRSes[i];
		const DirectX::XMMATRIX local =
			DirectX::XMMatrixScalingFromVector(DirectX::XMLoadFloat3(&trs.scale)) *
			DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&trs.rotation)) *
			DirectX::XMMatrixTranslationFromVector(DirectX::XMLoadFloat3(&trs.translation));
		DirectX::XMStoreFloat4x4(&localMatrices[i], local);
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
		const DirectX::XMMATRIX local = DirectX::XMLoadFloat4x4(&localMatrices[i]); // load to matrix from 4x4
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
