#include "CharacterData.h"

// Sci-Fi Dragon 4Wings (scifiDragon/Mesh/Sci-Fi Dragon 4Wings Skin 1.fbx)
// measurements: tools/scifiDragon_report.md, tools/scifiDragon_skeleton.txt
namespace
{
	HitShapeDesc Hurtbox(const char* name, const char* bone, DirectX::XMFLOAT3 localCenterB, float radius)
	{
		return {
			.role = HitVolumeRole::HURTBOX, .name = name, .boneName = bone,
			.localCenterB = localCenterB, .radius = radius
		};
	}

	// unused until the attacks are authored
	[[maybe_unused]] HitShapeDesc Hitbox(const char* name, const char* group, const char* bone,
	                                     DirectX::XMFLOAT3 localCenterA, DirectX::XMFLOAT3 localCenterB,
	                                     float radius, bool isSweeping)
	{
		return {
			.role = HitVolumeRole::HITBOX, .name = name, .groupName = group, .boneName = bone,
			.localCenterA = localCenterA, .localCenterB = localCenterB,
			.radius = radius, .isSweeping = isSweeping
		};
	}

	CharacterData BuildDragonData()
	{
		CharacterData data;
		data.scale = 0.015f; // model is in cm; 0.015 -> ~14.7 m long, ~18 m wingspan
		data.animDir = "../../assets/models/scifiDragon/Animations/";
		data.initialClip = "Idle1";

		// hurtboxes, measured from the bind pose at scale 0.015 (meters)
		// localCenterB = offset to the child joint in the bone's local frame; radius ~ 75th percentile of the
		// mesh distance around the bone. left bones point along local +X, right bones along -X (mirrored);
		// tail segments point along +Z (left) / -Z (right)
		data.shapes = {
			// body
			Hurtbox("body", "pelvis", {-1.05f, 0.f, 0.42f}, 0.75f), // hips back to the tail root
			Hurtbox("body", "spine_01", {0.96f, 0.f, 0.f}, 0.62f),
			Hurtbox("body", "spine_02", {0.92f, 0.f, -0.12f}, 0.81f),
			Hurtbox("chest", "spine_03", {0.85f, 0.f, 0.06f}, 0.99f),

			// neck and head
			Hurtbox("neck", "neck_01", {0.79f, 0.f, 0.f}, 0.52f),
			Hurtbox("neck", "neck_02", {0.83f, 0.f, 0.f}, 0.41f),
			Hurtbox("head", "head", {0.35f, 0.f, -1.f}, 0.6f), // skull toward the snout (local -Z), fitted to head vertex bounds

			// hind legs (no front legs: the wings are the arms)
			Hurtbox("legL", "thigh_l", {0.72f, 0.f, 0.f}, 0.41f),
			Hurtbox("legL", "calf_l", {0.72f, 0.f, 0.f}, 0.33f),
			Hurtbox("legL", "calf_l1", {0.9f, 0.f, 0.f}, 0.25f),
			Hurtbox("legL", "foot_l", {0.62f, 0.f, 0.f}, 0.18f),
			Hurtbox("legR", "thigh_r", {-0.72f, 0.f, 0.f}, 0.41f),
			Hurtbox("legR", "calf_r", {-0.72f, 0.f, 0.f}, 0.33f),
			Hurtbox("legR", "calf_r1", {-0.9f, 0.f, 0.f}, 0.25f),
			Hurtbox("legR", "foot_r", {-0.62f, 0.f, 0.f}, 0.18f),

			// wings: arm bones + leading edge + inner finger; fat radius on the fingers to cover the membrane
			// wingL1 / wingR1 = upper pair (upperarm_High_*)
			Hurtbox("wingL1", "upperarm_High_l", {1.48f, 0.f, 0.f}, 0.38f),
			Hurtbox("wingL1", "lowerarm_High_l", {2.28f, 0.f, 0.f}, 0.36f),
			Hurtbox("wingL1", "Hand_High_l", {0.22f, 1.99f, -0.09f}, 0.38f),
			Hurtbox("wingL1", "Wings_l", {2.08f, 0.f, 0.f}, 0.5f),
			Hurtbox("wingL1", "Wings_l1", {2.08f, 0.f, 0.f}, 0.5f),
			Hurtbox("wingL1", "Wings_l2", {1.99f, 0.f, 0.f}, 0.5f),
			Hurtbox("wingL1", "Wings_l3", {1.32f, 0.f, 0.f}, 0.5f),
			Hurtbox("wingL1", "Wings_l5", {1.35f, 0.f, 0.f}, 0.5f),
			Hurtbox("wingL1", "Wings_l6", {1.35f, 0.f, 0.f}, 0.5f),
			Hurtbox("wingL1", "Wings_l7", {1.89f, 0.f, 0.f}, 0.5f),

			Hurtbox("wingR1", "upperarm_High_r", {-1.48f, 0.f, 0.f}, 0.38f),
			Hurtbox("wingR1", "lowerarm_High_r", {-2.28f, 0.f, 0.f}, 0.36f),
			Hurtbox("wingR1", "Hand_High_r", {-0.22f, -1.99f, 0.09f}, 0.38f),
			Hurtbox("wingR1", "Wings_r", {-2.08f, 0.f, 0.f}, 0.5f),
			Hurtbox("wingR1", "Wings_r1", {-2.08f, 0.f, 0.f}, 0.5f),
			Hurtbox("wingR1", "Wings_r2", {-1.99f, 0.f, 0.f}, 0.5f),
			Hurtbox("wingR1", "Wings_r3", {-1.32f, 0.f, 0.f}, 0.5f),
			Hurtbox("wingR1", "Wings_r5", {-1.35f, 0.f, 0.f}, 0.5f),
			Hurtbox("wingR1", "Wings_r6", {-1.35f, 0.f, 0.f}, 0.5f),
			Hurtbox("wingR1", "Wings_r7", {-1.89f, 0.f, 0.f}, 0.5f),

			// wingL2 / wingR2 = lower pair (upperarm_Low_*)
			Hurtbox("wingL2", "upperarm_Low_l", {1.17f, 0.f, 0.f}, 0.32f),
			Hurtbox("wingL2", "lowerarm_Low_l", {2.15f, 0.f, 0.f}, 0.32f),
			Hurtbox("wingL2", "Hand_Low_l", {0.38f, 1.62f, -0.03f}, 0.27f),
			Hurtbox("wingL2", "Wings_l9", {1.21f, 0.f, 0.f}, 0.5f),
			Hurtbox("wingL2", "Wings_l10", {1.26f, 0.f, 0.f}, 0.5f),
			Hurtbox("wingL2", "Wings_l11", {1.58f, 0.f, 0.f}, 0.5f),
			Hurtbox("wingL2", "Wings_l12", {1.83f, 0.f, 0.f}, 0.5f),
			Hurtbox("wingL2", "Wings_l14", {1.03f, 0.f, 0.f}, 0.5f),
			Hurtbox("wingL2", "Wings_l15", {0.98f, 0.f, 0.f}, 0.5f),
			Hurtbox("wingL2", "Wings_l16", {1.12f, 0.f, 0.f}, 0.5f),

			Hurtbox("wingR2", "upperarm_Low_r", {-1.17f, 0.f, 0.f}, 0.32f),
			Hurtbox("wingR2", "lowerarm_Low_r", {-2.15f, 0.f, 0.f}, 0.32f),
			Hurtbox("wingR2", "Hand_Low_r", {-0.38f, -1.62f, 0.03f}, 0.27f),
			Hurtbox("wingR2", "Wings_r9", {-1.21f, 0.f, 0.f}, 0.5f),
			Hurtbox("wingR2", "Wings_r10", {-1.26f, 0.f, 0.f}, 0.5f),
			Hurtbox("wingR2", "Wings_r11", {-1.58f, 0.f, 0.f}, 0.5f),
			Hurtbox("wingR2", "Wings_r12", {-1.83f, 0.f, 0.f}, 0.5f),
			Hurtbox("wingR2", "Wings_r14", {-1.03f, 0.f, 0.f}, 0.5f),
			Hurtbox("wingR2", "Wings_r15", {-0.98f, 0.f, 0.f}, 0.5f),
			Hurtbox("wingR2", "Wings_r16", {-1.12f, 0.f, 0.f}, 0.5f),

			// twin tails, both from the shared "tail" root
			Hurtbox("tailL", "tail", {0.33f, 0.03f, 0.55f}, 0.45f),
			Hurtbox("tailL", "Tail_l1", {0.f, 0.f, 0.55f}, 0.29f),
			Hurtbox("tailL", "Tail_l2", {0.f, 0.f, 0.54f}, 0.27f),
			Hurtbox("tailL", "Tail_l3", {0.f, 0.f, 0.82f}, 0.26f),
			Hurtbox("tailL", "Tail_l4", {0.f, 0.f, 0.59f}, 0.25f),
			Hurtbox("tailL", "Tail_l5", {0.f, 0.f, 0.86f}, 0.3f),
			Hurtbox("tailL", "Tail_l6", {0.f, 0.f, 0.84f}, 0.25f),
			Hurtbox("tailL", "Tail_l7", {0.f, 0.f, 0.86f}, 0.25f),
			Hurtbox("tailL", "Tail_l8", {0.f, 0.f, 0.93f}, 0.2f),
			Hurtbox("tailL", "Tail_l9", {0.f, 0.f, 1.f}, 0.25f), // blade tip

			Hurtbox("tailR", "tail", {-0.33f, 0.03f, 0.55f}, 0.45f),
			Hurtbox("tailR", "Tail_r1", {0.f, 0.f, -0.55f}, 0.29f),
			Hurtbox("tailR", "Tail_r2", {0.f, 0.f, -0.54f}, 0.27f),
			Hurtbox("tailR", "Tail_r3", {0.f, 0.f, -0.82f}, 0.26f),
			Hurtbox("tailR", "Tail_r4", {0.f, 0.f, -0.59f}, 0.25f),
			Hurtbox("tailR", "Tail_r5", {0.f, 0.f, -0.86f}, 0.3f),
			Hurtbox("tailR", "Tail_r6", {0.f, 0.f, -0.84f}, 0.25f),
			Hurtbox("tailR", "Tail_r7", {0.f, 0.f, -0.86f}, 0.25f),
			Hurtbox("tailR", "Tail_r8", {0.f, 0.f, -0.93f}, 0.2f),
			Hurtbox("tailR", "Tail_r9", {0.f, 0.f, -1.f}, 0.25f), // blade tip

			// hitboxes: none yet (attacks get authored from the phase-1 motion report)

			// body blocker: horizontal capsule along the spine, from the hips to the chest;
			// radius reaches down to ~1.5 m so it overlaps the player's 1.8 m capsule
			HitShapeDesc{
				.role = HitVolumeRole::BODY, .name = "torso", .boneName = "spine_01",
				.localCenterA = {-0.7f, 0.f, 0.f}, .localCenterB = {2.f, 0.f, 0.f}, .radius = 1.f
			},
		};

		// animations
		data.clips = {
			// ground
			{"Idle1", true}, {"Idle2", true},
			{"WalkF", true}, {"WalkB", true}, {"WalkL", true}, {"WalkR", true},
			{"Turn90L", false}, {"Turn90R", false}, {"Turn180", false},
			{"Attk1", false}, {"Attk2", false}, {"Attk3", false}, {"Attk4", false}, {"Attk5", false},
			{"GetHitF", false}, {"GetHitF2", false}, {"GetHitB", false}, {"GetHitL", false}, {"GetHitR", false},
			{"Roar", false},
			{"Death1", false}, {"Death2", false},

			// ground <-> air
			{"TakeOff", false}, {"Land", false},

			// air (Idle3 is the hover idle)
			{"Idle3", true},
			{"FlyF", true}, {"FlyB", true}, {"FlyL", true}, {"FlyR", true},
			{"FlyAccelStart", false}, {"FlyAccelLoop", true}, {"FlyAccelEnd", false},
			{"FlyRollL", false}, {"FlyRollR", false},
			{"FlyTurn90L", false}, {"FlyTurn90R", false}, {"FlyTurn180", false},
			{"FlyAttk1", false}, {"FlyAttk2", false},
			{"FlyGetHit1", false}, {"FlyGetHit2", false},
		};

		// events: none yet (HitboxOn/Off come with the attacks; Footstep off for now)

		// fades
		const FadeDef fades[] = {
			{AnimationFadeTable::any, "Attk1", 0.1f},
			{AnimationFadeTable::any, "Attk2", 0.1f},
			{AnimationFadeTable::any, "Attk3", 0.1f},
			{AnimationFadeTable::any, "Attk4", 0.1f},
			{AnimationFadeTable::any, "Attk5", 0.1f},
			{AnimationFadeTable::any, "FlyAttk1", 0.1f},
			{AnimationFadeTable::any, "FlyAttk2", 0.1f},
			{AnimationFadeTable::any, "GetHitF", 0.05f},
			{AnimationFadeTable::any, "GetHitF2", 0.05f},
			{AnimationFadeTable::any, "GetHitB", 0.05f},
			{AnimationFadeTable::any, "GetHitL", 0.05f},
			{AnimationFadeTable::any, "GetHitR", 0.05f},
			{AnimationFadeTable::any, "FlyGetHit1", 0.05f},
			{AnimationFadeTable::any, "FlyGetHit2", 0.05f},
			{AnimationFadeTable::any, "Turn90L", 0.15f},
			{AnimationFadeTable::any, "Turn90R", 0.15f},
			{AnimationFadeTable::any, "Turn180", 0.15f},
			{"Idle1", "WalkF", 0.3f},
			{"WalkF", "Idle1", 0.3f},
		};
		auto fadeTable = std::make_shared<AnimationFadeTable>(0.2f);
		for (const FadeDef& fade : fades)
		{
			fadeTable->SetFade(fade.from, fade.to, fade.sec);
		}
		data.fadeTable = std::move(fadeTable);

		return data;
	}
}

const CharacterData& DragonData()
{
	static const CharacterData data = BuildDragonData();
	return data;
}
