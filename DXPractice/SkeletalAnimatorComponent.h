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
};

class SkeletalAnimatorComponent : public IComponent
{
public:
	SkeletalAnimatorComponent(Renderer& renderer, std::shared_ptr<const Skeleton> inSkeleton);

	void OnAttach(GameContext& context) override;
	void Update(float deltaTime) override;
	void BindPalette(Renderer& renderer);

	void SetCurrClip(std::shared_ptr<const AnimationClip> inClip, bool inLoop);
	void AddClip(std::shared_ptr<const AnimationClip> inClip, bool inLoop);
	void SetFadeTable(std::shared_ptr<const AnimationFadeTable> inTable)
	{
		fadeTable = inTable;
	}

	bool PlayClip(const std::string& name, float fadeSec = -1.f);

	bool IsFinished() const
	{
		return isFinished;
	}
	const std::string& GetCurrClipName() const;
	
	// sorted
	std::vector<std::string> GetClipNames() const;

	
	const Skeleton& GetSkeleton() const
	{
		return *skeleton;
	}

private:
	void AdvanceTime(float deltaTime);
	void SamplePose();
	void ComputeGlobals();
	void ComputePalette();

private:
	std::shared_ptr<const Skeleton> skeleton;
	std::vector<DirectX::XMFLOAT4X4> localPose;
	std::vector<DirectX::XMFLOAT4X4> globalPose;
	BonePalette palette{};
	VertexConstantBuffer<BonePalette> paletteCBuffer;

	std::unordered_map<std::string, ClipEntry> clipEntries;
	std::shared_ptr<const AnimationFadeTable> fadeTable;

	// animation playback states
	std::shared_ptr<const AnimationClip> currClip;
	std::vector<KeyCursor> cursors;
	float timeInSec = 0.f;
	bool isLoop = true;
	bool isFinished = false;
};
