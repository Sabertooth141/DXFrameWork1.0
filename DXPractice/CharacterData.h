#pragma once
#include <memory>
#include <string>
#include <vector>

#include "AnimationFadeTable.h"
#include "HitVolumeComponent.h"

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
