#include "ActionData.h"
#include "CharacterData.h"
#include "SkeletalAnimatorComponent.h"

// Sci-Fi Dragon 4Wings 
namespace
{
	void AddBreathCone(std::vector<HitShapeDesc>& outShapes, const char* group, const char* bone,
	                   DirectX::XMFLOAT3 dir, float startDist, float length, int count, float startRadius, float endRadius)
	{
		constexpr float overlap = 0.2f;

		const float len = sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
		const DirectX::XMFLOAT3 normalizedDir{dir.x / len, dir.y / len, dir.z / len};
		const auto at = [&](float distance)
		{
			return DirectX::XMFLOAT3{normalizedDir.x * distance, normalizedDir.y * distance, normalizedDir.z * distance};
		};

		const float segLen = length / static_cast<float>(count);
		for (int i = 0; i < count; i++)
		{
			float progress;
			if (count > 1)
			{
				progress = static_cast<float>(i) / static_cast<float>(count - 1);
			}
			else
			{
				progress = 0.f;
			}

			const float segStart = startDist + segLen * static_cast<float>(i);
			outShapes.push_back(CharacterHelpers::Hitbox(group, group, bone,
			                                             at(segStart), at(segStart + segLen + overlap),
			                                             startRadius + (endRadius - startRadius) * progress, false));
		}
	}

	CharacterData BuildDragonData()
	{
		CharacterData data;
		data.scale = 0.015f; // model is in cm; 0.015 -> ~14.7 m long, ~18 m wingspan
		data.modelRotationDeg = {0.f, 180.f, 0.f};
		data.animDir = "../../assets/models/scifiDragon/Animations/";
		data.initialClip = "Idle1";
		data.hitVolumePath = "../../data/hitvolumes/dragon.json";
		data.animSetPath = "../../data/animsets/dragon.json";

		data.rootMotionNode = "root";
		data.rootMotionClips = {
			{"Turn90L", RootMotionMode::YAW},
			{"Turn90R", RootMotionMode::YAW},
			{"Turn180", RootMotionMode::YAW}
		};

		return data;
	}

	std::vector<ActionDef> BuildDragonActions()
	{
		return {
			{.name = "Attk1", .clip = "Attk1", .fadeInSec = useFadeTable},
			{.name = "Attk2", .clip = "Attk2", .fadeInSec = useFadeTable},
			{.name = "Attk3", .clip = "Attk3", .fadeInSec = useFadeTable},
			{.name = "Attk4", .clip = "Attk4", .fadeInSec = useFadeTable},
			{.name = "Attk5", .clip = "Attk5", .fadeInSec = useFadeTable},
		};
	}
}

const CharacterData& DragonData()
{
	static const CharacterData data = BuildDragonData();
	return data;
}

const std::vector<ActionDef>& DragonActionData()
{
	static const std::vector<ActionDef> actionData = BuildDragonActions();
	return actionData;
}
