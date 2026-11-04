#pragma once
#include <DirectXMath.h>
#include <memory>

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

class SkeletalAnimatorComponent : public IComponent
{
public:
	SkeletalAnimatorComponent(Renderer& renderer, std::shared_ptr<const Skeleton> inSkeleton);

	void OnAttach(GameContext& context) override;
	void Update(float deltaTime) override;
	void BindPalette(Renderer& renderer);

	void SetClip(std::shared_ptr<const AnimationClip> inClip, bool inLoop);

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

	// animation playback states
	std::shared_ptr<const AnimationClip> currClip;
	std::vector<KeyCursor> cursors;
	float timeInSec = 0.f;
	bool isLoop = true;
	bool isFinished = false;
};
