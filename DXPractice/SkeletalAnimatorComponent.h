#pragma once
#include <DirectXMath.h>
#include <memory>

#include "AnimationFadeTable.h"
#include "ConstantBuffer.h"
#include "IComponent.h"
#include "Skeleton.h"

struct AnimationClip;

struct BonePalette
{
	DirectX::XMFLOAT4X4 bones[maxBones];
};

struct AnimationEvent
{
	float timeSec = 0.f;
	std::string eventName;
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

	bool PlayClip(const std::string& name, float fadeSec = -1.f);
	void AddEvent(const std::string& clipName, float timeSec, const std::string& eventName);

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

	const std::string& GetCurrClipName() const;
	
	// sorted
	std::vector<std::string> GetClipNames() const;

	
	const Skeleton& GetSkeleton() const
	{
		return *skeleton;
	}

private:
	void StartPlayback(const ClipEntry& entry);
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

	// for event firing push into pending and fire at the end of frame
	std::vector<FiredAnimationEvent> pendingEvents;
	std::vector<FiredAnimationEvent> firedEvents;
};
