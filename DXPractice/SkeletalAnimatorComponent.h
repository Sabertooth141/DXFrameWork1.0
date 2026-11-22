#pragma once
#include <DirectXMath.h>
#include <memory>

#include "AnimationFadeTable.h"
#include "ConstantBuffer.h"
#include "IComponent.h"
#include "Skeleton.h"

struct AnimationClip;

enum class RootMotionMode
{
	NONE,
	YAW,
	TRANSLATION_XZ
};

struct BonePalette
{
	DirectX::XMFLOAT4X4 bones[maxBones];
};

struct AnimationEvent
{
	float timeSec = 0.f;
	std::string eventName;
	std::string param; // group name for events
};

// one per channel
struct KeyCursor
{
	unsigned position = 0;
	unsigned rotation = 0;
	unsigned scale = 0;
};

struct ClipEntry
{
	std::shared_ptr<const AnimationClip> clip;
	bool isLoop = true;
	std::vector<AnimationEvent> events; // sorted by timeSec
	bool isExtractingRootYaw = false;
	RootMotionMode rootMotion = RootMotionMode::NONE;
};

struct ClipPlayback
{
	std::shared_ptr<const AnimationClip> clip;
	const std::vector<AnimationEvent>* events = nullptr;
	std::vector<KeyCursor> cursors;
	float timeInSec = 0.f;
	bool isLoop = true;
	bool isFinished = false;
	bool isFreshStart = false;
	bool didWrap = false;

	// root motion
	RootMotionMode rootMotion = RootMotionMode::NONE;
	int rootChannel = -1;
	float baseRootYaw = 0.f;
	float lastRootYaw = 0.f;
	DirectX::XMFLOAT3 firstRootPos{};
	DirectX::XMFLOAT3 endRootPos{};
	DirectX::XMFLOAT3 lastRootPos{};
	DirectX::XMFLOAT2 pinXZ{};
};

enum class FiredEventType
{
	MARKER,
	INTERRUPTED
};

struct FiredAnimationEvent
{
	FiredEventType type = FiredEventType::MARKER;
	std::string clipName;
	std::string eventName;
	float timeSec = 0.f;
	std::string param;
};

enum class FadeSource
{
	SNAPSHOT,
};

struct PoseFade
{
	bool isActive = false;
	FadeSource source = FadeSource::SNAPSHOT;
	float durationSec = 0.f;
	float elapsedSec = 0.f;
	std::vector<LocalTRS> fromPose;
};

class SkeletalAnimatorComponent : public IComponent
{
public:
	SkeletalAnimatorComponent(Renderer& renderer, std::shared_ptr<const Skeleton> inSkeleton);

	void OnAttach(GameContext& context) override;
	void Update(float deltaTime) override;
	void BindPalette(Renderer& renderer);

	void AddClip(std::shared_ptr<const AnimationClip> inClip, bool inLoop);

	void SetFadeTable(std::shared_ptr<const AnimationFadeTable> inTable)
	{
		fadeTable = inTable;
	}

	bool PlayClip(const std::string& name, float fadeSec = -1.f, float startTimeSec = 0.f);
	void AddEvent(const std::string& clipName, float timeSec, const std::string& eventName, const std::string& param = {});

	// for data-driven setup / the editor (AnimSetComponent)
	bool HasClip(const std::string& name) const
	{
		return clipEntries.contains(name);
	}

	bool HasEvent(const std::string& clipName, const std::string& eventName) const;
	bool WasMarkerFired(const std::string& eventName) const;

	float GetClipLength(const std::string& name) const; // seconds, 0 if unknown
	void SetClipLoop(const std::string& name, bool inLoop); // applies the next time the clip is played
	void ClearEvents(); // every clip's markers, before re-adding them

	// root motion
	void SetRootMotionNode(const std::string& nodeName);
	void SetClipRootMotion(const std::string& name, RootMotionMode mode);
	void SetRootMotionAnchor(const std::string& clipName);

	float GetRootYawDelta() const
	{
		return rootYawDelta;
	}

	const DirectX::XMFLOAT3& GetRootMoveDelta() const
	{
		return rootMoveDelta;
	}

	bool IsExtractingRootYaw() const
	{
		return playback.rootMotion == RootMotionMode::YAW;
	}

	bool IsExtractingRootMove() const
	{
		return playback.rootMotion == RootMotionMode::TRANSLATION_XZ;
	}

	float GetExtractedYawTotal() const
	{
		return extractedYawTotal;
	}

	float GetExtractedMoveTotal() const
	{
		return sqrtf(extractedMoveTotal.x * extractedMoveTotal.x + extractedMoveTotal.z * extractedMoveTotal.z);
	}

	const std::vector<FiredAnimationEvent>& GetFiredEvents() const
	{
		return firedEvents;
	}

	float GetCurrClipTime() const
	{
		return playback.timeInSec;
	}

	float GetCurrClipLength() const;

	// debugging, for marker setting
	void Scrub(float deltaSec);

	bool IsFinished() const
	{
		return playback.isFinished;
	}

	bool IsFading() const
	{
		return fade.isActive;
	}

	void SetTimeScale(float inScale)
	{
		timeScale = inScale;
	}

	float GetTimeScale() const
	{
		return timeScale;
	}

	const std::string& GetCurrClipName() const;

	// sorted
	std::vector<std::string> GetClipNames() const;


	const Skeleton& GetSkeleton() const
	{
		return *skeleton;
	}

	int FindNodeIndex(const std::string& name) const;

	DirectX::XMMATRIX GetNodeWorldMatrix(int nodeIndex) const;
	DirectX::XMMATRIX GetBoneWorldMatrix(const std::string& name) const;
	void SetModelRotation(const DirectX::XMFLOAT3& inRotation);

private:
	void StartPlayback(const ClipEntry& entry, float startTimeSec = 0.f);
	void StartFade(float fadeSec);
	static void AdvanceTime(ClipPlayback& clipPlayback, float deltaTime);
	void SampleClip(ClipPlayback& clipPlayback, std::vector<LocalTRS>& outPose);
	void UpdateFade(float deltaTime);
	void BuildLocalMatrices();
	void ComputeGlobals();
	void ComputePalette();
	void Tick(float clipDeltaSec);
	void CollectEvents(float prevTimeSec, float deltaSec);
	void ClampToRange(float startSec, float finishSec, bool isIncludingStartSec);

	void ExtractRootYaw(ClipPlayback& clipPlayback, std::vector<LocalTRS>& outPose);
	void ExtractRootMove(ClipPlayback& clipPlayback, std::vector<LocalTRS>& outPose);
	DirectX::XMFLOAT3 RootMoveToWorld(const DirectX::XMFLOAT3& parentSpaceDelta) const;

private:
	std::shared_ptr<const Skeleton> skeleton;

	std::vector<LocalTRS> currPoseTRSes;
	std::vector<LocalTRS> sampledPoseTRSes;
	std::vector<DirectX::XMFLOAT4X4> localMatrices;
	std::vector<DirectX::XMFLOAT4X4> globalPose;
	BonePalette palette{};
	VertexConstantBuffer<BonePalette> paletteCBuffer;

	std::unordered_map<std::string, ClipEntry> clipEntries;
	std::shared_ptr<const AnimationFadeTable> fadeTable;

	// animation playback states
	ClipPlayback playback;
	PoseFade fade;
	float timeScale = 1.f; // for slow mo
	DirectX::XMFLOAT4X4 modelCorrection{
		1.f, 0.f, 0.f, 0.f,
		0.f, 1.f, 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		0.f, 0.f, 0.f, 1.f
	};

	// for event firing push into pending and fire at the end of frame
	std::vector<FiredAnimationEvent> pendingEvents;
	std::vector<FiredAnimationEvent> firedEvents;

	// root motion
	int rootMotionNodeIndex = -1;
	float rootYawDelta = 0.f;
	float extractedYawTotal = 0.f;

	bool hasRootAnchor = false;
	DirectX::XMFLOAT2 rootAnchorXZ{};
	DirectX::XMFLOAT3 rootMoveLocal{};
	DirectX::XMFLOAT3 rootMoveDelta{};
	DirectX::XMFLOAT3 extractedMoveTotal{};
};
