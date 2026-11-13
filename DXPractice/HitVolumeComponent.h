#pragma once
#include <DirectXMath.h>
#include <string>
#include <vector>

#include "Collision3D.h"
#include "IComponent.h"

class SkeletalAnimatorComponent;
class DebugRenderer;

enum class HitVolumeRole
{
	HURTBOX,
	HITBOX
};

struct HitShapeDesc
{
	HitVolumeRole role = HitVolumeRole::HURTBOX;
	std::string name;
	std::string groupName;
	std::string boneName;

	DirectX::XMFLOAT3 localCenterA{};
	DirectX::XMFLOAT3 localCenterB{}; // == A for sphere
	float radius = 0.5f;
	bool isSweeping = false; // for fast moving hitbox
};

struct HitShape
{
	HitShapeDesc desc;
	int nodeIndex = -1; // -1 == object root

	Capsule3D world{};
	Capsule3D prevWorld{};
	bool hasWorld = false;
	bool hasPrev = false;

	// for debug
	DirectX::XMFLOAT3 bonePos{};
	DirectX::XMFLOAT4 boneRot{ 0.f, 0.f, 0.f, 1.f };
};

class HitVolumeComponent : public IComponent
{
public:
	void OnAttach(GameContext& context) override;

	// returns shape index
	int AddShape(const HitShapeDesc& desc);

	void UpdateWorldShapes();

	const std::vector<HitShape>& GetShapes() const
	{
		return hitShapes;
	}

	void DrawDebug(DebugRenderer& debugRenderer) const;

public:
	bool isShowingBoneAxes = true;

private:
	std::vector<HitShape> hitShapes;
	const SkeletalAnimatorComponent* animator = nullptr;
};

