#pragma once
#include <DirectXMath.h>
#include <memory>

#include "ConstantBuffer.h"
#include "IComponent.h"
#include "Skeleton.h"

struct BonePalette
{
	DirectX::XMFLOAT4X4 bones[maxBones];
};

class SkeletalAnimatorComponent : public IComponent
{
public:
	SkeletalAnimatorComponent(Renderer& renderer, std::shared_ptr<const Skeleton> inSkeleton);

	void BindPalette(Renderer& renderer);

private:
	void ComputeGloabls();
	void ComputePalette();

private:
	std::shared_ptr<const Skeleton> skeleton;
	std::vector<DirectX::XMFLOAT4X4> localPose;
	std::vector<DirectX::XMFLOAT4X4> globalPose;
	BonePalette palette{};
	VertexConstantBuffer<BonePalette> paletteCBuffer;
};
