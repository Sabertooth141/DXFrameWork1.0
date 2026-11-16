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

// per-group settings that live in the data file
struct HitGroupDesc
{
	std::string name;
	float rehitIntervalSec = 0.f; // 0 = once per activation
};

// everything a HitVolumeComponent is built from (what the JSON file holds)
struct HitVolumeDesc
{
	std::vector<HitShapeDesc> shapes;
	std::vector<HitGroupDesc> groups;
};

struct HitShape
{
	HitShapeDesc desc;
	int nodeIndex = -1; // -1 == object root
	int groupIndex = -1; // hitboxes only
	bool isEnabled = true; // hurtboxes only
	bool isBoneMissing = false; // boneName not in the skeleton -> follows the root instead

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

	// replace every shape and group (editor edits, file loads)
	void Build(const HitVolumeDesc& desc);
	// current shapes + the groups used by hitboxes, in a form that can be saved or edited
	HitVolumeDesc ExportDesc() const;

	// data file: loads path if it exists, otherwise builds from fallback (code-defined data).
	// The path is remembered either way, so the editor's Save creates the file.
	void LoadFile(const std::string& path, const HitVolumeDesc& fallback);
	bool ReloadFile(std::string& outMessage); // editor "Revert"
	bool SaveFile(std::string& outMessage) const;

	const std::string& GetDataPath() const
	{
		return dataPath;
	}

	const std::string& GetLoadMessage() const
	{
		return loadMessage;
	}

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
	int highlightedShape = -1; // editor selection, drawn in yellow

private:
	int FindGroup(const std::string& inGroupName) const;
	int FindOrAddGroup(const std::string& inGroupName);
	void ActivateGroup(int groupIndex, bool isActive, const std::string& clipName);

private:
	std::vector<HitShape> hitShapes;
	std::vector<HitGroup> hitGroups;
	const SkeletalAnimatorComponent* animator = nullptr;

	std::string dataPath; // empty = defined in code only
	std::string loadMessage;
};