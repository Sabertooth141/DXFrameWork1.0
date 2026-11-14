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
	HITBOX,
	BODY
};

struct HitRecord
{
	const GameObject* victim = nullptr;
	float lastHitTimeSec = 0.f;
};

struct HitGroup
{
	std::string name;
	bool isActive = false;
	unsigned activationCount = 0;
	float rehitIntervalSec = 0.f; // 0 = once per activation
	std::string activatedByClip;
	std::vector<HitRecord> hitRecords;
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
	int groupIndex = -1; // hitboxes only
	bool isEnabled = true; // hurtboxes only

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

	// hitbox groups
	void SetGroupActive(const std::string& groupName, bool inIsActive);
	bool IsGroupActive(const std::string& groupName);
	void SetGroupRehitInterval(const std::string& groupName, float intervalSec);

	// hurtboxes
	void SetHurtboxEnabled(const std::string& name, bool inIsEnabled);
	void SetAllHurtboxEnabled(bool inIsEnabled);

	// hitboxOn / hitboxOff
	void ApplyAnimationEvents();

	void UpdateWorldShapes();

	bool CanHit(int groupIndex, const GameObject* victim, float currTime) const;
	void RecordHit(int groupIndex, const GameObject* victim, float currTime);

	const std::vector<HitShape>& GetShapes() const
	{
		return hitShapes;
	}

	const std::vector<HitGroup>& GetGroups() const
	{
		return hitGroups;
	}

	void DrawDebug(DebugRenderer& debugRenderer) const;

public:
	bool isShowingBoneAxes = true;

private:
	int FindGroup(const std::string& inGroupName) const;
	int FindOrAddGroup(const std::string& inGroupName);
	void ActivateGroup(int groupIndex, bool isActive, const std::string& clipName);

private:
	std::vector<HitShape> hitShapes;
	std::vector<HitGroup> hitGroups;
	const SkeletalAnimatorComponent* animator = nullptr;
};

