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

	float YawOf(DirectX::FXMVECTOR rotation)
	{
		return 2.f * atan2(DirectX::XMVectorGetY(rotation), DirectX::XMVectorGetW(rotation));
	}

	int FindNodeChannel(const AnimationClip& clip, int nodeIndex)
	{
		for (size_t i = 0; i < clip.channels.size(); i++)
		{
			if (clip.channelToNode[i] == nodeIndex)
			{
				return static_cast<int>(i);
			}
		}

		return -1;
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
	if (clipEntries.contains(name))
	{
		OutputDebugStringA(("[SkeletalAnimator] clip name '" + name + "' already registered, overwriting\n").c_str());
		assert(false && "duplicate clip name");
	}
	clipEntries[name] = {std::move(inClip), inLoop};
}

bool SkeletalAnimatorComponent::PlayClip(const std::string& name, float fadeSec, float startTimeSec)
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
	StartPlayback(it->second, startTimeSec);
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

bool SkeletalAnimatorComponent::HasEvent(const std::string& clipName, const std::string& eventName) const
{
	const auto it = clipEntries.find(clipName);
	if (it == clipEntries.end())
	{
		return false;
	}

	return std::ranges::any_of(it->second.events, [&eventName](const AnimationEvent& event)
	{
		return eventName == event.eventName;
	});
}

bool SkeletalAnimatorComponent::WasMarkerFired(const std::string& eventName) const
{
	const std::string& currClipName = GetCurrClipName();
	return std::ranges::any_of(firedEvents, [&](const FiredAnimationEvent& event)
	{
		return event.type == FiredEventType::MARKER && event.eventName == eventName && event.clipName == currClipName;
	});
}

float SkeletalAnimatorComponent::GetClipLength(const std::string& name) const
{
	const auto it = clipEntries.find(name);
	return it != clipEntries.end() && it->second.clip ? it->second.clip->DurationInSeconds() : 0.f;
}

void SkeletalAnimatorComponent::SetClipLoop(const std::string& name, bool inLoop)
{
	if (const auto it = clipEntries.find(name); it != clipEntries.end())
	{
		it->second.isLoop = inLoop;
	}
}

void SkeletalAnimatorComponent::ClearEvents()
{
	// clear() keeps each vector object alive, so playback.events (a pointer to the playing
	// clip's vector) stays valid and simply sees the new markers after they are re-added
	for (auto& [name, entry] : clipEntries)
	{
		entry.events.clear();
	}
}

void SkeletalAnimatorComponent::SetRootMotionNode(const std::string& nodeName)
{
	rootMotionNodeIndex = FindNodeIndex(nodeName);
}

void SkeletalAnimatorComponent::SetClipRootMotion(const std::string& name, RootMotionMode mode)
{
	const auto it = clipEntries.find(name);
	if (it == clipEntries.end())
	{
		OutputDebugStringA(("[SkeletalAnimator] SetClipRootMotion: no clip named '" + name + "'\n").c_str());
		return;
	}

	it->second.rootMotion = mode;
}

void SkeletalAnimatorComponent::SetRootMotionAnchor(const std::string& clipName)
{
	hasRootAnchor = false;

	const auto it = clipEntries.find(clipName);
	if (it == clipEntries.end() || rootMotionNodeIndex < 0)
	{
		OutputDebugStringA(("[SkeletalAnimator] SetRootMotionAnchor: no clip '" + clipName + "' or no root node\n").c_str());
		return;
	}

	const AnimationClip& clip = *it->second.clip;
	const int channel = FindNodeChannel(clip, rootMotionNodeIndex);
	if (channel < 0 || clip.channels[channel].positions.empty())
	{
		OutputDebugStringA(("[SkeletalAnimator] SetRootMotionAnchor: '" + clipName + "' has no root position keys\n").c_str());
		return;
	}

	const DirectX::XMFLOAT3& restRootPos = clip.channels[channel].positions.front().value;
	rootAnchorXZ = {restRootPos.x, restRootPos.z};
	hasRootAnchor = true;
}

void SkeletalAnimatorComponent::ExtractRootMove(ClipPlayback& clipPlayback, std::vector<LocalTRS>& outPose)
{
	LocalTRS& rootTRS = outPose[rootMotionNodeIndex];
	const DirectX::XMFLOAT3 pos = rootTRS.translation;

	float deltaX = pos.x - clipPlayback.lastRootPos.x;
	float deltaZ = pos.z - clipPlayback.lastRootPos.z;
	if (clipPlayback.didWrap)
	{
		// if looping = start -> now
		deltaX = (clipPlayback.endRootPos.x - clipPlayback.lastRootPos.x) + (pos.x - clipPlayback.firstRootPos.x);
		deltaZ = (clipPlayback.endRootPos.z - clipPlayback.lastRootPos.z) + (pos.z - clipPlayback.firstRootPos.z);
	}
	clipPlayback.lastRootPos = pos;

	rootMoveLocal.x += deltaX;
	rootMoveLocal.z += deltaZ;

	rootTRS.translation.x = clipPlayback.pinXZ.x;
	rootTRS.translation.z = clipPlayback.pinXZ.y;
}

DirectX::XMFLOAT3 SkeletalAnimatorComponent::RootMoveToWorld(const DirectX::XMFLOAT3& parentSpaceDelta) const
{
	const int parentIndex = skeleton->nodes[rootMotionNodeIndex].parent;
	DirectX::XMMATRIX parentGlobal;
	if (parentIndex >= 0)
	{
		parentGlobal = DirectX::XMLoadFloat4x4(&globalPose[parentIndex]);
	}
	else
	{
		parentGlobal = DirectX::XMMatrixIdentity();
	}

	const DirectX::XMMATRIX toWorld =
		parentGlobal *
		DirectX::XMLoadFloat4x4(&skeleton->meshSpaceInverse) *
		DirectX::XMLoadFloat4x4(&modelCorrection) *
		owner->GetTransform()->GetMatrix();

	DirectX::XMFLOAT3 out;
	DirectX::XMStoreFloat3(&out, DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&parentSpaceDelta), toWorld));
	out.y = 0.f;
	return out;
}

void SkeletalAnimatorComponent::ExtractRootYaw(ClipPlayback& clipPlayback, std::vector<LocalTRS>& outPose)
{
	LocalTRS& rootTRS = outPose[rootMotionNodeIndex];
	const DirectX::XMVECTOR rotation = DirectX::XMLoadFloat4(&rootTRS.rotation);
	const float yaw = YawOf(rotation);

	const float delta = remainderf(yaw - clipPlayback.lastRootYaw, DirectX::XM_2PI);
	clipPlayback.lastRootYaw = yaw;
	rootYawDelta += delta;
	extractedYawTotal += delta;

	// keep the yaw the clip started with, drop what it turned since (tilt and roll stay)
	const DirectX::XMVECTOR undoTurn =
		DirectX::XMQuaternionRotationRollPitchYaw(0.f, -(yaw - clipPlayback.baseRootYaw), 0.f);
	DirectX::XMStoreFloat4(&rootTRS.rotation, DirectX::XMQuaternionNormalize(DirectX::XMQuaternionMultiply(rotation, undoTurn)));
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
	rootYawDelta = 0.f;
	rootMoveLocal = {};
	rootMoveDelta = {};

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

	if (playback.rootMotion == RootMotionMode::TRANSLATION_XZ)
	{
		rootMoveDelta = RootMoveToWorld(rootMoveLocal);
		extractedMoveTotal.x += rootMoveDelta.x;
		extractedMoveTotal.z += rootMoveDelta.z;
	}
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
		DirectX::XMLoadFloat4x4(&modelCorrection) *
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

void SkeletalAnimatorComponent::SetModelRotation(const DirectX::XMFLOAT3& inRotation)
{
	const DirectX::XMMATRIX rotation =
		DirectX::XMMatrixRotationRollPitchYaw(inRotation.x, inRotation.y, inRotation.z);
	DirectX::XMStoreFloat4x4(&modelCorrection, rotation);

	// the palette may not be rebuilt by tick,
	// so rebuild it now from the current globals
	ComputePalette();

	// keep the editors pick box facing the same way as the mesh
	owner->SetBoundsCorrection(rotation);
}

void SkeletalAnimatorComponent::StartPlayback(const ClipEntry& entry, float startTimeSec)
{
	assert(entry.clip->builtFor == skeleton.get() && "clip was mapped agaisnt a different skelton");

	playback.clip = entry.clip;
	playback.events = &entry.events;
	playback.isLoop = entry.isLoop;
	playback.timeInSec = std::clamp(startTimeSec, 0.f, entry.clip->DurationInSeconds());
	playback.isFinished = false;
	playback.isFreshStart = true;
	playback.cursors.assign(playback.clip->channels.size(), KeyCursor{});

	// root motion
	playback.rootMotion = RootMotionMode::NONE;
	playback.rootChannel = -1;
	playback.didWrap = false;
	extractedYawTotal = 0.f;
	extractedMoveTotal = {};

	if (entry.rootMotion == RootMotionMode::NONE || rootMotionNodeIndex < 0)
	{
		return;
	}

	const AnimationClip& clip = *entry.clip;
	const int channel = FindNodeChannel(clip, rootMotionNodeIndex);
	if (channel < 0)
	{
		return;
	}

	const AnimationChannel& rootChannel = clip.channels[channel];
	if (entry.rootMotion == RootMotionMode::YAW && !rootChannel.rotations.empty())
	{
		const DirectX::XMFLOAT4& firstKey = rootChannel.rotations.front().value;
		playback.baseRootYaw = YawOf(DirectX::XMLoadFloat4(&firstKey));
		playback.lastRootYaw = playback.baseRootYaw;
		playback.rootChannel = channel;
		playback.rootMotion = RootMotionMode::YAW;
	}
	else if (entry.rootMotion == RootMotionMode::TRANSLATION_XZ && !rootChannel.positions.empty())
	{
		playback.firstRootPos = rootChannel.positions.front().value;
		playback.endRootPos = rootChannel.positions.back().value;

		unsigned cursor = 0;
		const float startTicks = playback.timeInSec * clip.ticksPerSecond;
		DirectX::XMStoreFloat3(&playback.lastRootPos, SampleVectorKeyInfo(rootChannel.positions, startTicks, cursor, DirectX::XMLoadFloat3(&playback.firstRootPos)));

		playback.pinXZ = hasRootAnchor
			                 ? rootAnchorXZ
			                 : DirectX::XMFLOAT2{playback.firstRootPos.x, playback.firstRootPos.z};
		playback.rootChannel = channel;
		playback.rootMotion = RootMotionMode::TRANSLATION_XZ;
	}
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
	clipPlayback.didWrap = false;

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
		clipPlayback.didWrap = deltaTime > 0.f && clipPlayback.timeInSec >= length;
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

	switch (clipPlayback.rootMotion)
	{
	case RootMotionMode::YAW:
		ExtractRootYaw(clipPlayback, outPose);
		break;
	case RootMotionMode::TRANSLATION_XZ:
		ExtractRootMove(clipPlayback, outPose);
		break;
	case RootMotionMode::NONE:
		break;
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
	DirectX::XMLoadFloat4x4(&skeleton->meshSpaceInverse);
	const DirectX::XMMATRIX toModel =
		DirectX::XMLoadFloat4x4(&skeleton->meshSpaceInverse) *
		DirectX::XMLoadFloat4x4(&modelCorrection);

	for (size_t boneIndex = 0; boneIndex < skeleton->bones.size(); boneIndex++)
	{
		const SkeletonBone& bone = skeleton->bones[boneIndex];
		const DirectX::XMMATRIX finalMatrix =
			DirectX::XMLoadFloat4x4(&bone.offset) *
			DirectX::XMLoadFloat4x4(&globalPose[bone.node]) *
			toModel;

		DirectX::XMStoreFloat4x4(&palette.bones[boneIndex], DirectX::XMMatrixTranspose(finalMatrix));
	}
}
