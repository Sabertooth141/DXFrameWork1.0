#include "CharacterData.h"

// Usurper dragon (DragonUsurperMesh.fbx)
namespace
{
	HitShapeDesc Hurtbox(const char* name, const char* bone, DirectX::XMFLOAT3 localCenterB, float radius)
	{
		return {
			.role = HitVolumeRole::HURTBOX, .name = name, .boneName = bone,
			.localCenterB = localCenterB, .radius = radius
		};
	}

	HitShapeDesc Hitbox(const char* name, const char* group, const char* bone,
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
		data.scale = 0.01f;
		data.animDir = "../../assets/models/dragon/animations/";
		data.initialClip = "Idle2";

		// hit volumes, measured from DragonUsurperMesh.fbx (meters, bind pose)
		// almost every bone points along its local +X, so "along the bone" = {length, 0, 0}
		data.shapes = {
			// body
			Hurtbox("body", "Spine01", {1.14f, 0.f, 0.f}, 1.1f),
			Hurtbox("body", "Spine02", {1.28f, 0.f, 0.f}, 1.3f),
			Hurtbox("chest", "Chest", {1.27f, 0.f, 0.f}, 1.2f),

			// neck and head
			Hurtbox("neck", "Neck01", {1.22f, 0.f, 0.f}, 0.9f),
			Hurtbox("neck", "Neck02", {1.13f, 0.f, 0.f}, 0.8f),
			Hurtbox("neck", "Neck03", {1.05f, 0.f, 0.f}, 0.7f),
			Hurtbox("head", "Head", {0.f, 0.f, -1.9f}, 0.7f),

			// front legs
			Hurtbox("armL", "UpperArm_L", {1.76f, 0.f, 0.f}, 0.7f),
			Hurtbox("armL", "LowerArm_L", {1.72f, 0.f, 0.f}, 0.45f),
			Hurtbox("armR", "UpperArm_R", {1.76f, 0.f, 0.f}, 0.7f),
			Hurtbox("armR", "RowerArm_R", {1.72f, 0.f, 0.f}, 0.45f), // typo'd in the asset

			// hind legs
			Hurtbox("legL", "UpperLeg_L", {1.79f, 0.f, 0.f}, 0.65f),
			Hurtbox("legL", "LowerLeg_L", {1.56f, 0.f, 0.f}, 0.4f),
			Hurtbox("legR", "UpperReg_R", {1.79f, 0.f, 0.f}, 0.65f), // typo'd
			Hurtbox("legR", "RowerReg_R", {1.56f, 0.f, 0.f}, 0.4f), // typo'd

			// tail
			Hurtbox("tail", "joint19", {1.51f, 0.f, 0.f}, 0.85f),
			Hurtbox("tail", "joint20", {1.54f, 0.f, 0.f}, 0.75f),
			Hurtbox("tail", "joint21", {1.96f, 0.f, 0.f}, 0.6f),
			Hurtbox("tail", "joint22", {1.99f, 0.f, 0.f}, 0.5f),
			Hurtbox("tail", "joint23", {0.f, 0.f, 1.4f}, 0.35f),

			// hitboxes: one group per attack

			// lunge (AttkHand): the body is the weapon
			Hitbox("body", "lunge", "Spine02", {}, {1.28f, 0.f, 0.f}, 1.3f, false),
			Hitbox("body", "lunge", "Chest", {}, {1.27f, 0.f, 0.f}, 1.2f, false),
			Hitbox("neck", "lunge", "Neck01", {}, {1.22f, 0.f, 0.f}, 0.9f, false),
			Hitbox("head", "lunge", "Head", {}, {0.f, 0.f, -1.9f}, 0.7f, false),
			Hitbox("arm", "lunge", "UpperArm_L", {}, {1.76f, 0.f, 0.f}, 0.7f, false),
			Hitbox("arm", "lunge", "UpperArm_R", {}, {1.76f, 0.f, 0.f}, 0.7f, false),
			Hitbox("forearm", "lunge", "LowerArm_L", {}, {1.72f, 0.f, 0.f}, 0.45f, true),
			Hitbox("forearm", "lunge", "RowerArm_R", {}, {1.72f, 0.f, 0.f}, 0.45f, true), // typo'd
			Hitbox("claw", "lunge", "Hand_L", {}, {1.4f, 0.f, 0.f}, 0.45f, true),
			Hitbox("claw", "lunge", "Hand_R", {}, {1.4f, 0.f, 0.f}, 0.45f, true),

			// bite (AttkMouth): head + lower jaw, swept because the head snaps fast
			Hitbox("head", "bite", "Head", {}, {0.f, 0.f, -1.9f}, 0.7f, true),
			Hitbox("jaw", "bite", "Jaw01", {}, {1.32f, 0.f, 0.f}, 0.4f, true),

			// breath (AttkFlame, FlyFlame): widening chain along the snout (UpperMouth01 +X); length and radii are guesses
			Hitbox("fire", "breath", "UpperMouth01", {1.f, 0.f, 0.f}, {4.f, 0.f, 0.f}, 0.6f, false),
			Hitbox("fire", "breath", "UpperMouth01", {4.f, 0.f, 0.f}, {8.f, 0.f, 0.f}, 1.f, false),
			Hitbox("fire", "breath", "UpperMouth01", {8.f, 0.f, 0.f}, {12.f, 0.f, 0.f}, 1.4f, false),

			// wingL (AttkWing): leading edge + finger bones, fat radius to cover the membrane between fingers
			Hitbox("wing", "wingL", "Wing01_L", {}, {2.99f, 0.f, 0.f}, 0.5f, true),
			Hitbox("wing", "wingL", "Wing02_L", {}, {4.69f, 0.f, 0.f}, 0.5f, true),
			Hitbox("wing", "wingL", "WingIndex02_L", {}, {2.28f, 0.f, 0.f}, 0.6f, true),
			Hitbox("wing", "wingL", "WingMiddle02_L", {}, {3.71f, 0.f, 0.f}, 0.6f, true),
			Hitbox("wing", "wingL", "WingMiddle04_L", {}, {5.40f, 0.f, 0.f}, 0.6f, true),
			Hitbox("wing", "wingL", "WingRing04_L", {}, {4.07f, 0.f, 0.f}, 0.6f, true),
			Hitbox("wing", "wingL", "WingPinky04_L", {}, {2.82f, 0.f, 0.f}, 0.6f, true),

			// body (pushbox)
			HitShapeDesc{
				.role = HitVolumeRole::BODY, .name = "torso", .boneName = "Spine01",
				.localCenterA = {0.f, 0.f, 0.f}, .localCenterB = {3.7f, 0.f, 0.f}, .radius = 1.2f
			},
		};

		data.rehits = {
			{"breath", 0.2f},
		};

		// animations
		data.clips = {
			{"Idle1", true}, {"Idle2", true}, {"Walk", true},
			{"Run", true},
			{"AttkHand", false}, {"AttkMouth", false},
			{"AttkFlame", false}, {"AttkWing", false},
			{"GetHit", false}, {"Die", false},
			{"Scream", false},
			{"Sleep", true}, {"TakeOff", false}, {"Land", false},
			{"FlyIdle", true}, {"FlyForward", true}, {"FlyGlide", true},
			{"FlyFlame", true},
		};

		// events, seconds on each clip's own timeline
		// TODO: manual timing manipulation
		data.events = {
			// AttkHand
			{"AttkHand", 1.2f, "HitboxOn", "lunge"},
			{"AttkHand", 1.65f, "HitboxOff", "lunge"},
			// AttkMouth
			{"AttkMouth", 0.5f, "HitboxOn", "bite"},
			{"AttkMouth", 1.1f, "HitboxOff", "bite"},
			// AttkWing
			{"AttkWing", 0.8f, "HitboxOn", "wingL"},
			{"AttkWing", 1.2f, "HitboxOff", "wingL"},

			// locomotions
			// walk
			{"Walk", 0.f, "Footstep"},
			{"Walk", 0.64f, "Footstep"},
		};

		// fades
		const FadeDef fades[] = {
			{AnimationFadeTable::any, "AttkHand", 0.1f},
			{AnimationFadeTable::any, "AttkMouth", 0.1f},
			{AnimationFadeTable::any, "AttkFlame", 0.1f},
			{AnimationFadeTable::any, "GetHit", 0.05f},
			{"Idle1", "Walk", 0.3f},
			{"Walk", "Idle1", 0.3f},
			{"Walk", "Run", 0.3f},
			{"Run", "Walk", 0.3f},
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
