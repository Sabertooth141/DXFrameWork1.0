#include "CharacterData.h"

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
		data.animDir = "../../assets/models/scifiDragon/Animations/";
		data.initialClip = "Idle1";

		// hurtboxes, measured from the bind pose at scale 0.015 (meters)
		// localCenterB = offset to the child joint in the bone's local frame; radius ~ 75th percentile of the
		// mesh distance around the bone. left bones point along local +X, right bones along -X (mirrored);
		// tail segments point along +Z (left) / -Z (right)
		data.shapes = {
			// body
			CharacterHelpers::Hurtbox("body", "pelvis", {-1.05f, 0.f, 0.42f}, 0.75f), // hips back to the tail root
			CharacterHelpers::Hurtbox("body", "spine_01", {0.96f, 0.f, 0.f}, 0.62f),
			CharacterHelpers::Hurtbox("body", "spine_02", {0.92f, 0.f, -0.12f}, 0.81f),
			CharacterHelpers::Hurtbox("chest", "spine_03", {0.85f, 0.f, 0.06f}, 0.99f),

			// neck and head
			CharacterHelpers::Hurtbox("neck", "neck_01", {0.79f, 0.f, 0.f}, 0.52f),
			CharacterHelpers::Hurtbox("neck", "neck_02", {0.83f, 0.f, 0.f}, 0.41f),
			CharacterHelpers::Hurtbox("head", "head", {0.35f, 0.f, -1.f}, 0.6f), // skull toward the snout (local -Z), fitted to head vertex bounds

			// hind legs (no front legs: the wings are the arms)
			CharacterHelpers::Hurtbox("legL", "thigh_l", {0.72f, 0.f, 0.f}, 0.41f),
			CharacterHelpers::Hurtbox("legL", "calf_l", {0.72f, 0.f, 0.f}, 0.33f),
			CharacterHelpers::Hurtbox("legL", "calf_l1", {0.9f, 0.f, 0.f}, 0.25f),
			CharacterHelpers::Hurtbox("legL", "foot_l", {0.62f, 0.f, 0.f}, 0.18f),
			CharacterHelpers::Hurtbox("legR", "thigh_r", {-0.72f, 0.f, 0.f}, 0.41f),
			CharacterHelpers::Hurtbox("legR", "calf_r", {-0.72f, 0.f, 0.f}, 0.33f),
			CharacterHelpers::Hurtbox("legR", "calf_r1", {-0.9f, 0.f, 0.f}, 0.25f),
			CharacterHelpers::Hurtbox("legR", "foot_r", {-0.62f, 0.f, 0.f}, 0.18f),

			// wings: arm bones + leading edge + inner finger; fat radius on the fingers to cover the membrane
			// wingL1 / wingR1 = upper pair (upperarm_High_*)
			CharacterHelpers::Hurtbox("wingL1", "upperarm_High_l", {1.48f, 0.f, 0.f}, 0.38f),
			CharacterHelpers::Hurtbox("wingL1", "lowerarm_High_l", {2.28f, 0.f, 0.f}, 0.36f),
			CharacterHelpers::Hurtbox("wingL1", "Hand_High_l", {0.22f, 1.99f, -0.09f}, 0.38f),
			CharacterHelpers::Hurtbox("wingL1", "Wings_l", {2.08f, 0.f, 0.f}, 0.5f),
			CharacterHelpers::Hurtbox("wingL1", "Wings_l1", {2.08f, 0.f, 0.f}, 0.5f),
			CharacterHelpers::Hurtbox("wingL1", "Wings_l2", {1.99f, 0.f, 0.f}, 0.5f),
			CharacterHelpers::Hurtbox("wingL1", "Wings_l3", {1.32f, 0.f, 0.f}, 0.5f),
			CharacterHelpers::Hurtbox("wingL1", "Wings_l5", {1.35f, 0.f, 0.f}, 0.5f),
			CharacterHelpers::Hurtbox("wingL1", "Wings_l6", {1.35f, 0.f, 0.f}, 0.5f),
			CharacterHelpers::Hurtbox("wingL1", "Wings_l7", {1.89f, 0.f, 0.f}, 0.5f),

			CharacterHelpers::Hurtbox("wingR1", "upperarm_High_r", {-1.48f, 0.f, 0.f}, 0.38f),
			CharacterHelpers::Hurtbox("wingR1", "lowerarm_High_r", {-2.28f, 0.f, 0.f}, 0.36f),
			CharacterHelpers::Hurtbox("wingR1", "Hand_High_r", {-0.22f, -1.99f, 0.09f}, 0.38f),
			CharacterHelpers::Hurtbox("wingR1", "Wings_r", {-2.08f, 0.f, 0.f}, 0.5f),
			CharacterHelpers::Hurtbox("wingR1", "Wings_r1", {-2.08f, 0.f, 0.f}, 0.5f),
			CharacterHelpers::Hurtbox("wingR1", "Wings_r2", {-1.99f, 0.f, 0.f}, 0.5f),
			CharacterHelpers::Hurtbox("wingR1", "Wings_r3", {-1.32f, 0.f, 0.f}, 0.5f),
			CharacterHelpers::Hurtbox("wingR1", "Wings_r5", {-1.35f, 0.f, 0.f}, 0.5f),
			CharacterHelpers::Hurtbox("wingR1", "Wings_r6", {-1.35f, 0.f, 0.f}, 0.5f),
			CharacterHelpers::Hurtbox("wingR1", "Wings_r7", {-1.89f, 0.f, 0.f}, 0.5f),

			// wingL2 / wingR2 = lower pair (upperarm_Low_*)
			CharacterHelpers::Hurtbox("wingL2", "upperarm_Low_l", {1.17f, 0.f, 0.f}, 0.32f),
			CharacterHelpers::Hurtbox("wingL2", "lowerarm_Low_l", {2.15f, 0.f, 0.f}, 0.32f),
			CharacterHelpers::Hurtbox("wingL2", "Hand_Low_l", {0.38f, 1.62f, -0.03f}, 0.27f),
			CharacterHelpers::Hurtbox("wingL2", "Wings_l9", {1.21f, 0.f, 0.f}, 0.5f),
			CharacterHelpers::Hurtbox("wingL2", "Wings_l10", {1.26f, 0.f, 0.f}, 0.5f),
			CharacterHelpers::Hurtbox("wingL2", "Wings_l11", {1.58f, 0.f, 0.f}, 0.5f),
			CharacterHelpers::Hurtbox("wingL2", "Wings_l12", {1.83f, 0.f, 0.f}, 0.5f),
			CharacterHelpers::Hurtbox("wingL2", "Wings_l14", {1.03f, 0.f, 0.f}, 0.5f),
			CharacterHelpers::Hurtbox("wingL2", "Wings_l15", {0.98f, 0.f, 0.f}, 0.5f),
			CharacterHelpers::Hurtbox("wingL2", "Wings_l16", {1.12f, 0.f, 0.f}, 0.5f),

			CharacterHelpers::Hurtbox("wingR2", "upperarm_Low_r", {-1.17f, 0.f, 0.f}, 0.32f),
			CharacterHelpers::Hurtbox("wingR2", "lowerarm_Low_r", {-2.15f, 0.f, 0.f}, 0.32f),
			CharacterHelpers::Hurtbox("wingR2", "Hand_Low_r", {-0.38f, -1.62f, 0.03f}, 0.27f),
			CharacterHelpers::Hurtbox("wingR2", "Wings_r9", {-1.21f, 0.f, 0.f}, 0.5f),
			CharacterHelpers::Hurtbox("wingR2", "Wings_r10", {-1.26f, 0.f, 0.f}, 0.5f),
			CharacterHelpers::Hurtbox("wingR2", "Wings_r11", {-1.58f, 0.f, 0.f}, 0.5f),
			CharacterHelpers::Hurtbox("wingR2", "Wings_r12", {-1.83f, 0.f, 0.f}, 0.5f),
			CharacterHelpers::Hurtbox("wingR2", "Wings_r14", {-1.03f, 0.f, 0.f}, 0.5f),
			CharacterHelpers::Hurtbox("wingR2", "Wings_r15", {-0.98f, 0.f, 0.f}, 0.5f),
			CharacterHelpers::Hurtbox("wingR2", "Wings_r16", {-1.12f, 0.f, 0.f}, 0.5f),

			// twin tails, both from the shared "tail" root
			CharacterHelpers::Hurtbox("tailL", "tail", {0.33f, 0.03f, 0.55f}, 0.45f),
			CharacterHelpers::Hurtbox("tailL", "Tail_l1", {0.f, 0.f, 0.55f}, 0.29f),
			CharacterHelpers::Hurtbox("tailL", "Tail_l2", {0.f, 0.f, 0.54f}, 0.27f),
			CharacterHelpers::Hurtbox("tailL", "Tail_l3", {0.f, 0.f, 0.82f}, 0.26f),
			CharacterHelpers::Hurtbox("tailL", "Tail_l4", {0.f, 0.f, 0.59f}, 0.25f),
			CharacterHelpers::Hurtbox("tailL", "Tail_l5", {0.f, 0.f, 0.86f}, 0.3f),
			CharacterHelpers::Hurtbox("tailL", "Tail_l6", {0.f, 0.f, 0.84f}, 0.25f),
			CharacterHelpers::Hurtbox("tailL", "Tail_l7", {0.f, 0.f, 0.86f}, 0.25f),
			CharacterHelpers::Hurtbox("tailL", "Tail_l8", {0.f, 0.f, 0.93f}, 0.2f),
			CharacterHelpers::Hurtbox("tailL", "Tail_l9", {0.f, 0.f, 1.f}, 0.25f), // blade tip

			CharacterHelpers::Hurtbox("tailR", "tail", {-0.33f, 0.03f, 0.55f}, 0.45f),
			CharacterHelpers::Hurtbox("tailR", "Tail_r1", {0.f, 0.f, -0.55f}, 0.29f),
			CharacterHelpers::Hurtbox("tailR", "Tail_r2", {0.f, 0.f, -0.54f}, 0.27f),
			CharacterHelpers::Hurtbox("tailR", "Tail_r3", {0.f, 0.f, -0.82f}, 0.26f),
			CharacterHelpers::Hurtbox("tailR", "Tail_r4", {0.f, 0.f, -0.59f}, 0.25f),
			CharacterHelpers::Hurtbox("tailR", "Tail_r5", {0.f, 0.f, -0.86f}, 0.3f),
			CharacterHelpers::Hurtbox("tailR", "Tail_r6", {0.f, 0.f, -0.84f}, 0.25f),
			CharacterHelpers::Hurtbox("tailR", "Tail_r7", {0.f, 0.f, -0.86f}, 0.25f),
			CharacterHelpers::Hurtbox("tailR", "Tail_r8", {0.f, 0.f, -0.93f}, 0.2f),
			CharacterHelpers::Hurtbox("tailR", "Tail_r9", {0.f, 0.f, -1.f}, 0.25f), // blade tip

			// hitboxes:
			// Attk5: left wing slam, upper pair leading edge (same offsets as the wingL1 hurtboxes).
			// wing tip peaks ~167 m/s -> sweeping
			CharacterHelpers::Hitbox("wingL", "wingL", "upperarm_High_l", {}, {1.48f, 0.f, 0.f}, 1.f, true),
			CharacterHelpers::Hitbox("wingL", "wingL", "lowerarm_High_l", {}, {2.28f, 0.f, 0.f}, 1.f, true),
			CharacterHelpers::Hitbox("wingL", "wingL", "Hand_High_l", {}, {0.22f, 1.99f, -0.09f}, 1.f, true),
			CharacterHelpers::Hitbox("wingL", "wingL", "Wings_l", {}, {2.08f, 0.f, 0.f}, 1.f, true),
			CharacterHelpers::Hitbox("wingL", "wingL", "Wings_l1", {}, {2.08f, 0.f, 0.f}, 1.f, true),
			CharacterHelpers::Hitbox("wingL", "wingL", "Wings_l2", {}, {1.99f, 0.f, 0.f}, 1.f, true),
			CharacterHelpers::Hitbox("wingL", "wingL", "Wings_l3", {}, {1.32f, 0.f, 0.f}, 1.f, true),
			// Attk5: lower left wing, same group so one slam = one hit (offsets = wingL2 hurtboxes)
			CharacterHelpers::Hitbox("wingL", "wingL", "upperarm_Low_l", {}, { 1.17f, 0.f, 0.f }, 0.8f, true),
			CharacterHelpers::Hitbox("wingL", "wingL", "lowerarm_Low_l", {}, { 2.15f, 0.f, 0.f }, 0.8f, true),
			CharacterHelpers::Hitbox("wingL", "wingL", "Hand_Low_l", {}, { 0.38f, 1.62f, -0.03f }, 0.6f, true),
			CharacterHelpers::Hitbox("wingL", "wingL", "Wings_l9", {}, { 1.21f, 0.f, 0.f }, 1.f, true),
			CharacterHelpers::Hitbox("wingL", "wingL", "Wings_l10", {}, { 1.26f, 0.f, 0.f }, 1.f, true),
			CharacterHelpers::Hitbox("wingL", "wingL", "Wings_l11", {}, { 1.58f, 0.f, 0.f }, 1.f, true),
			CharacterHelpers::Hitbox("wingL", "wingL", "Wings_l12", {}, { 1.83f, 0.f, 0.f }, 1.f, true),


			// body blocker: horizontal capsule along the spine, from the hips to the chest;
			// radius reaches down to ~1.5 m so it overlaps the player's 1.8 m capsule
			HitShapeDesc{
				.role = HitVolumeRole::BODY, .name = "torso", .boneName = "spine_01",
				.localCenterA = {-0.7f, 0.f, 0.f}, .localCenterB = {2.f, 0.f, 0.f}, .radius = 1.f
			},
		};

		// // snout axis in the head bone's frame (same direction as the head hurtbox)
		const DirectX::XMFLOAT3 snoutDir{0.33f, 0.f, -0.94f};
		// Attk3: head used as a blade, past the snout tip (~1.6 m), thin and fast
		data.shapes.push_back(CharacterHelpers::Hitbox("headBlade", "headBlade", "head",
		                                               {snoutDir.x * 0.3f, 0.f, snoutDir.z * 0.3f},
		                                               {snoutDir.x * 6.f, 0.f, snoutDir.z * 6.f}, 0.25f, true));

		// Attk1 / Attk4 / FlyAttk1 / FlyAttk2: shared breath cone from the head.
		// 1.2 m (just past the jaw) to 11.2 m, 5 capsules, radius 0.4 -> 2.0
		AddBreathCone(data.shapes, "breath", "head", snoutDir, 1.2f, 10.f, 5, 0.4f, 2.f);

		data.rehits = {
			{"breath", 0.4f}, // continuous: re-hit every 0.4 s
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

		// events: 
		data.events = {
			{"Attk1", 1.17f, "HitboxOn", "breath"}, {"Attk1", 1.80f, "HitboxOff", "breath"},
			{"Attk3", 1.13f, "HitboxOn", "headBlade"}, {"Attk3", 2.f, "HitboxOff", "headBlade"},
			{"Attk4", 1.67f, "HitboxOn", "breath"}, {"Attk4", 2.27f, "HitboxOff", "breath"},
			{"Attk5", 0.83f, "HitboxOn", "wingL"}, {"Attk5", 1.13f, "HitboxOff", "wingL"},
			{"FlyAttk1", 0.8f, "HitboxOn", "breath"}, {"FlyAttk1", 2.f, "HitboxOff", "breath"},
			{"FlyAttk2", 0.7f, "HitboxOn", "breath"}, {"FlyAttk2", 2.8f, "HitboxOff", "breath"},
		};

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
