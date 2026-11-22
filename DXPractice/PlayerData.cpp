#include "ActionData.h"
#include "CharacterData.h"
#include "SkeletalAnimatorComponent.h"


// 10E, The Violinist: UE nodachi clips retargeted with Maya HumanIK
namespace
{
	CharacterData BuildPlayerData()
	{
		CharacterData data;
		data.scale = 0.82f; // ~2.1 m tall at 1.0 -> ~1.75 m
		data.modelRotationDeg = {0.f, 0.f, 180.f}; // file flips the skeleton 180 deg on x
		data.animDir = "../../assets/models/10e/animations/";
		data.initialClip = "IdleCombat";
		data.hitVolumePath = "../../data/hitvolumes/player.json";
		data.animSetPath = "../../data/animsets/player.json";

		// fallback until player.json is saved: one hurtbox from the shins to the head (metres, not scaled)
		data.shapes = {
			{
				.role = HitVolumeRole::HURTBOX, .name = "body",
				.localCenterA = {0.f, 0.35f, 0.f}, .localCenterB = {0.f, 1.4f, 0.f}, .radius = 0.3f
			}
		};

		data.clips = {
			{"IdleCombat", true},
			{"RunStart", false}, {"RunLoop", true}, {"RunStop", false},
			{"RunFastStart", false}, {"RunFastLoop", true}, {"RunFastStop", false},
			{"RollF_RM", false},
			{"RunAttk1_RM", false},
			{"Combo1Attk1_RM", false}, {"Combo1Attk2_RM", false}, {"Combo1Attk3_RM", false}, {"Combo1Attk4_RM", false},
			{"Combo2Attk1_RM", false}, {"Combo2Attk2_RM", false}, {"Combo2Attk3_RM", false}, {"Combo2Attk4_RM", false},
			{"Combo3Attk1", false}, {"Combo3Attk2", false}, {"Combo3Attk3", false}, {"Combo3Attk4", false},
			{"Death_RM", false}, {"GetUpB", false},
		};

		data.rootMotionNode = "spine";
		data.rootMotionAnchorClip = "IdleCombat";
		for (const ClipDef& clip : data.clips)
		{
			if (std::string_view(clip.name).ends_with("_RM"))
			{
				data.rootMotionClips.push_back({clip.name, RootMotionMode::TRANSLATION_XZ});
			}
		}

		data.rootMotionClips.push_back({ "GetUpB", RootMotionMode::TRANSLATION_XZ });

		return data;
	}

	std::vector<ActionDef> BuildPlayerActions()
	{
		return {
			// openers fade in from whatever loop is playing; chained hits start on an identical frame -> no fade
			{.name = "Combo1Attk1", .clip = "Combo1Attk1_RM", .nextOnAttack = "Combo1Attk2"},
			{.name = "Combo1Attk2", .clip = "Combo1Attk2_RM", .nextOnAttack = "Combo1Attk3", .fadeInSec = 0.f},
			{.name = "Combo1Attk3", .clip = "Combo1Attk3_RM", .nextOnAttack = "Combo1Attk4", .fadeInSec = 0.f},
			{.name = "Combo1Attk4", .clip = "Combo1Attk4_RM", .fadeInSec = 0.f},

			{.name = "Combo3Attk1", .clip = "Combo3Attk1", .nextOnAttack2 = "Combo3Attk2"},
			{.name = "Combo3Attk2", .clip = "Combo3Attk2", .nextOnAttack2 = "Combo3Attk3", .fadeInSec = 0.f},
			{.name = "Combo3Attk3", .clip = "Combo3Attk3", .nextOnAttack2 = "Combo3Attk4", .fadeInSec = 0.f},
			{.name = "Combo3Attk4", .clip = "Combo3Attk4", .fadeInSec = 0.f},

			{.name = "RollF", .clip = "RollF_RM"},
		};
	}
}

const CharacterData& PlayerData()
{
	static const CharacterData data = BuildPlayerData();
	return data;
}

const std::vector<ActionDef>& PlayerActionData()
{
	static const std::vector<ActionDef> actions = BuildPlayerActions();
	return actions;
}
