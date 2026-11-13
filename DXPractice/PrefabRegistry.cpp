#include "PrefabRegistry.h"

#include <memory>

#include "AnimationClip.h"
#include "AnimationClipCache.h"
#include "BoxCollider2D.h"
#include "CycleAnimationTester.h"
#include "GameObject.h"
#include "HitVolumeComponent.h"
#include "PhysicsTest.h"
#include "SkeletalAnimatorComponent.h"

namespace
{
	std::shared_ptr<const AnimationFadeTable> DragonFadeTable()
	{
		static const std::shared_ptr<const AnimationFadeTable> fadeTable = []
		{
			auto table = std::make_shared<AnimationFadeTable>(0.2f);
			table->SetFade(AnimationFadeTable::any, "attackHand", 0.1f);
			table->SetFade(AnimationFadeTable::any, "attackMouth", 0.1f);
			table->SetFade(AnimationFadeTable::any, "attackFlame", 0.1f);
			table->SetFade(AnimationFadeTable::any, "getHit", 0.05f);
			table->SetFade("idle01", "Walk", 0.3f);
			table->SetFade("Walk", "idle01", 0.3f);
			table->SetFade("Walk", "Run", 0.3f);
			table->SetFade("Run", "Walk", 0.3f);
			return table;
		}();

		return fadeTable;
	}
}

void RegisterPrefabs()
{
	// blocks
	PrefabRegistry::Instance().RegisterSprite("block", [](GameObject& object)
	{
		auto& anim = object.AddComponent<AnimatorComponent>(object.GetRenderer());
		anim.SetRenderLayer(RenderLayer::Default);
		anim.SetSortOrder(0);
		anim.SetStatic(L"../../assets/Sprites/jinx.jpg");

		object.GetTransform()->SetScale(0.05f);

		auto& rb = object.AddComponent<Rigidbody2DComponent>(*object.GetTransform(), 1.0f);
		rb.SetGravity(500.f);

		object.AddComponent<BoxCollider2D>(DirectX::XMFLOAT2(0.5f, 0.5f), DirectX::XMFLOAT2(0.f, 0.f), false,
		                                   *object.GetTransform());

		object.AddComponent<PhysicsTest>();
		object.SetTag(ObjectTag::BLOCK);
	});

	// testCube
	MaterialData mat{};
	mat.specularIntensity = 0.5f;
	mat.specularPower = 2.f;
	PrefabRegistry::Instance().RegisterModel("testCube", "../../assets/models/testCube.fbx", mat, [](GameObject& object)
	{
	});

	// test light
	PrefabRegistry::Instance().RegisterEmtpy("testLight", [](GameObject& obj)
	{
		LightComponent& lightComp = obj.AddComponent<LightComponent>();
		lightComp.color = {1.f, 0.8f, 0.6f};
	});

	// stage
	PrefabRegistry::Instance().RegisterModel("stage", "../../assets/models/Stage.fbx", mat, [](GameObject& object)
	{
	});

	// dragon
	PrefabRegistry::Instance().RegisterModel("dragon", "../../assets/models/dragon/DragonUsurperMesh.fbx", mat, [](GameObject& object)
	{
		object.GetTransform()->SetScale(0.01f);

		auto* animator = object.GetComponent<SkeletalAnimatorComponent>();
		const std::string animDir = "../../assets/models/dragon/animations/";

		// hit volumes, measured from DragonUsurperMesh.fbx (meters, bind pose)
		// almost every bone points along its local +X, so "along the bone" = {length, 0, 0}
		auto& hitVolume = object.AddComponent<HitVolumeComponent>();
		auto addHurtbox = [&hitVolume](const char* name, const char* bone, DirectX::XMFLOAT3 localCenterB, float radius)
		{
			hitVolume.AddShape({
				.role = HitVolumeRole::HURTBOX, .name = name, .boneName = bone,
				.localCenterB = localCenterB, .radius = radius
			});
		};

		// body
		addHurtbox("body", "Spine01", {1.14f, 0.f, 0.f}, 1.1f);
		addHurtbox("body", "Spine02", {1.28f, 0.f, 0.f}, 1.3f);
		addHurtbox("chest", "Chest", {1.27f, 0.f, 0.f}, 1.2f);

		// neck and head 
		addHurtbox("neck", "Neck01", {1.22f, 0.f, 0.f}, 0.9f);
		addHurtbox("neck", "Neck02", {1.13f, 0.f, 0.f}, 0.8f);
		addHurtbox("neck", "Neck03", {1.05f, 0.f, 0.f}, 0.7f);
		addHurtbox("head", "Head", {0.f, 0.f, -1.9f}, 0.7f);

		// front legs
		addHurtbox("armL", "UpperArm_L", {1.76f, 0.f, 0.f}, 0.7f);
		addHurtbox("armL", "LowerArm_L", {1.72f, 0.f, 0.f}, 0.45f);
		addHurtbox("armR", "UpperArm_R", {1.76f, 0.f, 0.f}, 0.7f);
		addHurtbox("armR", "RowerArm_R", {1.72f, 0.f, 0.f}, 0.45f); // typo'd in the asset

		// hind legs
		addHurtbox("legL", "UpperLeg_L", {1.79f, 0.f, 0.f}, 0.65f);
		addHurtbox("legL", "LowerLeg_L", {1.56f, 0.f, 0.f}, 0.4f);
		addHurtbox("legR", "UpperReg_R", {1.79f, 0.f, 0.f}, 0.65f); // typo'd
		addHurtbox("legR", "RowerReg_R", {1.56f, 0.f, 0.f}, 0.4f); // typo'd

		// tail 
		addHurtbox("tail", "joint19", {1.51f, 0.f, 0.f}, 0.85f);
		addHurtbox("tail", "joint20", {1.54f, 0.f, 0.f}, 0.75f);
		addHurtbox("tail", "joint21", {1.96f, 0.f, 0.f}, 0.6f);
		addHurtbox("tail", "joint22", {1.99f, 0.f, 0.f}, 0.5f);
		addHurtbox("tail", "joint23", {0.f, 0.f, 1.4f}, 0.35f);

		// hitboxes: one group per attack
		auto addHitbox = [&hitVolume](const char* name, const char* group, const char* bone,
		                              DirectX::XMFLOAT3 localCenterA, DirectX::XMFLOAT3 localCenterB,
		                              float radius, bool isSweeping)
		{
			hitVolume.AddShape({
				.role = HitVolumeRole::HITBOX, .name = name, .groupName = group, .boneName = bone,
				.localCenterA = localCenterA, .localCenterB = localCenterB,
				.radius = radius, .isSweeping = isSweeping
			});
		};

		// lunge (attackHand): the body is the weapon
		addHitbox("body", "lunge", "Spine02", {}, {1.28f, 0.f, 0.f}, 1.3f, false);
		addHitbox("body", "lunge", "Chest", {}, {1.27f, 0.f, 0.f}, 1.2f, false);
		addHitbox("neck", "lunge", "Neck01", {}, {1.22f, 0.f, 0.f}, 0.9f, false);
		addHitbox("head", "lunge", "Head", {}, {0.f, 0.f, -1.9f}, 0.7f, false);
		addHitbox("arm", "lunge", "UpperArm_L", {}, {1.76f, 0.f, 0.f}, 0.7f, false);
		addHitbox("arm", "lunge", "UpperArm_R", {}, {1.76f, 0.f, 0.f}, 0.7f, false);
		addHitbox("forearm", "lunge", "LowerArm_L", {}, { 1.72f, 0.f, 0.f }, 0.45f, true);
		addHitbox("forearm", "lunge", "RowerArm_R", {}, { 1.72f, 0.f, 0.f }, 0.45f, true); // typo'd
		addHitbox("claw", "lunge", "Hand_L", {}, { 1.4f, 0.f, 0.f }, 0.45f, true);
		addHitbox("claw", "lunge", "Hand_R", {}, { 1.4f, 0.f, 0.f }, 0.45f, true);

		// bite (attackMouth): head + lower jaw, swept because the head snaps fast
		addHitbox("head", "bite", "Head", {}, {0.f, 0.f, -1.9f}, 0.7f, true);
		addHitbox("jaw", "bite", "Jaw01", {}, {1.32f, 0.f, 0.f}, 0.4f, true);

		// breath (attackFlame, FlyFlame): widening chain along the snout (UpperMouth01 +X); length and radii are guesses
		addHitbox("fire", "breath", "UpperMouth01", {1.f, 0.f, 0.f}, {4.f, 0.f, 0.f}, 0.6f, false);
		addHitbox("fire", "breath", "UpperMouth01", {4.f, 0.f, 0.f}, {8.f, 0.f, 0.f}, 1.f, false);
		addHitbox("fire", "breath", "UpperMouth01", {8.f, 0.f, 0.f}, {12.f, 0.f, 0.f}, 1.4f, false);

		// wingL (attackWing): leading edge + finger bones, fat radius to cover the membrane between fingers
		addHitbox("wing", "wingL", "Wing01_L", {}, {2.99f, 0.f, 0.f}, 0.5f, true);
		addHitbox("wing", "wingL", "Wing02_L", {}, {4.69f, 0.f, 0.f}, 0.5f, true);
		addHitbox("wing", "wingL", "WingIndex02_L", {}, {2.28f, 0.f, 0.f}, 0.6f, true);
		addHitbox("wing", "wingL", "WingMiddle02_L", {}, {3.71f, 0.f, 0.f}, 0.6f, true);
		addHitbox("wing", "wingL", "WingMiddle04_L", {}, {5.40f, 0.f, 0.f}, 0.6f, true);
		addHitbox("wing", "wingL", "WingRing04_L", {}, {4.07f, 0.f, 0.f}, 0.6f, true);
		addHitbox("wing", "wingL", "WingPinky04_L", {}, {2.82f, 0.f, 0.f}, 0.6f, true);

		// animations
		struct ClipDef
		{
			const char* name;
			bool loop;
		};
		for (const ClipDef& def : {
			     ClipDef{"idle01", true}, {"idle02", true}, {"Walk", true},
			     {"Run", true},
			     {"attackHand", false}, {"attackMouth", false},
			     {"attackFlame", false}, {"attackWing", false},
			     {"getHit", false}, {"Die", false},
			     {"Scream", false},
			     {"Sleep", true}, {"takeOff", false}, {"Land", false},
			     {"FlyIdle", true}, {"FlyForward", true}, {"FlyGlide", true},
			     {"FlyFlame", true}
		     })
		{
			animator->AddClip(
				AnimationClipCache::LoadClip(
					animDir + def.name + ".fbx", animator->GetSkeleton()),
				def.loop);
		}
		animator->SetFadeTable(DragonFadeTable());

		// events, seconds on each clip's own timeline
		// TODO: manual timing manipulation
		animator->AddEvent("attackHand", 1.2f, "HitboxOn", "lunge");
		animator->AddEvent("attackHand", 1.65f, "HitboxOff", "lunge");
		animator->AddEvent("Walk", 0.f, "Footstep");
		animator->AddEvent("Walk", 0.64f, "Footstep");


		animator->PlayClip("idle02");

		object.AddComponent<CycleAnimationTester>();
	});
}
