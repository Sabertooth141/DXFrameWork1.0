#pragma once
#include <memory>
#include <string>
#include <vector>

#include "AnimationFadeTable.h"
#include "HitVolumeComponent.h"

namespace CharacterHelpers
{
	inline HitShapeDesc Hurtbox(const char* name, const char* bone, DirectX::XMFLOAT3 localCenterB, float radius)
	{
		return {
			.role = HitVolumeRole::HURTBOX, .name = name, .boneName = bone,
			.localCenterB = localCenterB, .radius = radius
		};
	}

	// unused until the attacks are authored
	[[maybe_unused]] inline HitShapeDesc Hitbox(const char* name, const char* group, const char* bone,
	                                            DirectX::XMFLOAT3 localCenterA, DirectX::XMFLOAT3 localCenterB,
	                                            float radius, bool isSweeping)
	{
		return {
			.role = HitVolumeRole::HITBOX, .name = name, .groupName = group, .boneName = bone,
			.localCenterA = localCenterA, .localCenterB = localCenterB,
			.radius = radius, .isSweeping = isSweeping
		};
	}
}

class GameObject;

struct ClipDef
{
	const char* name;
	bool loop;
};

// seconds on the clip's own timeline
struct EventDef
{
	const char* clip;
	float timeSec;
	const char* event;
	const char* param = "";
};

struct FadeDef
{
	const char* from;
	const char* to;
	float sec;
};

struct GroupRehitDef
{
	const char* group;
	float intervalSec;
};

struct CharacterData
{
	float scale = 1.f;
	std::string animDir; // clip path = animDir + name + ".fbx"
	std::string initialClip;

	std::vector<HitShapeDesc> shapes;
	std::vector<GroupRehitDef> rehits;

	std::vector<ClipDef> clips;
	std::vector<EventDef> events;
	std::shared_ptr<const AnimationFadeTable> fadeTable;
};

// needs the SkeletalAnimatorComponent already on the object (RegisterModel with a skinned mesh)
void ApplyCharacterData(GameObject& object, const CharacterData& data);

// character tables, each built once on first call
const CharacterData& DragonData(); // DragonData.cpp
