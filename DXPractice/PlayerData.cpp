#include "CharacterData.h"
#include "SkeletalAnimatorComponent.h"

// 10E, The Violinist: UE nodachi clips retargeted with Maya HumanIK
namespace
{
	CharacterData BuildPlayerData()
	{
		CharacterData data;
		data.scale = 0.82f; // ~2.1 m tall at 1.0 -> ~1.75 m
		data.modelRotationDeg = { 0.f, 0.f, 180.f }; // file flips the skeleton 180 deg on x
		data.animDir = "../../assets/models/10e/animations/";
		data.initialClip = "IdleCombat";
		data.hitVolumePath = "../../data/hitvolumes/player.json";
		data.animSetPath = "../../data/animsets/player.json";

		// fallback until player.json is saved: one hurtbox from the shins to the head (metres, not scaled)
		data.shapes = {
			{
				.role = HitVolumeRole::HURTBOX, .name = "body",
				.localCenterA = { 0.f, 0.35f, 0.f }, .localCenterB = { 0.f, 1.4f, 0.f }, .radius = 0.3f
			}
		};

		data.clips = {
			{ "IdleCombat", true },
			{ "RunStart", false },{ "RunLoop", true },{ "RunStop", false },
			{ "RunFastStart", false },{ "RunFastLoop", true },{ "RunFastStop", false },
			{ "RollF_RM", false },
			{ "RunAttk1_RM", false },
			{ "Combo1Attk1_RM", false },{ "Combo1Attk2_RM", false },{ "Combo1Attk3_RM", false },{ "Combo1Attk4_RM", false },
			{ "Combo2Attk1_RM", false },{ "Combo2Attk2_RM", false },{ "Combo2Attk3_RM", false },{ "Combo2Attk4_RM", false },
			{ "Combo3Attk1", false },{ "Combo3Attk2", false },{ "Combo3Attk3", false },{ "Combo3Attk4", false },
			{ "HitLargeF_RM", false },
			{ "Death_RM", false },{ "GetUpB", false },
		};

		data.rootMotionNode = "spine";
		data.rootMotionAnchorClip = "IdleCombat";
		for (const ClipDef& clip : data.clips)
		{
			if (std::string_view(clip.name).ends_with("_RM"))
			{
				data.rootMotionClips.push_back({ clip.name, RootMotionMode::TRANSLATION_XZ });
			}
		}

		return data;
	}
}

const CharacterData& PlayerData()
{
	static const CharacterData data = BuildPlayerData();
	return data;
}