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

		// neck and head (Head's X is sideways; its +Z points to the snout)
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

		// tail (joint23's axes match the world; the tip continues along its -Z)
		addHurtbox("tail", "joint19", {1.51f, 0.f, 0.f}, 0.85f);
		addHurtbox("tail", "joint20", {1.54f, 0.f, 0.f}, 0.75f);
		addHurtbox("tail", "joint21", {1.96f, 0.f, 0.f}, 0.6f);
		addHurtbox("tail", "joint22", {1.99f, 0.f, 0.f}, 0.5f);
		//addHurtbox("tail", "joint23", {0.f, 0.f, -1.4f}, 0.35f);

		// claw hitbox (attackHand): hand to claw tips, plus the forearm
		hitVolume.AddShape({
			.role = HitVolumeRole::HITBOX, .name = "claw", .groupName = "claw", .boneName = "Hand_R",
			.localCenterB = {1.4f, 0.f, 0.f}, .radius = 0.45f, .isSweeping = true
		});
		hitVolume.AddShape({
			.role = HitVolumeRole::HITBOX, .name = "forearm", .groupName = "claw", .boneName = "RowerArm_R",
			.localCenterB = {1.72f, 0.f, 0.f}, .radius = 0.45f, .isSweeping = true
		});
		hitVolume.AddShape({
			.role = HitVolumeRole::HITBOX, .name = "claw", .groupName = "clawL", .boneName = "Hand_L",
			.localCenterB = {1.4f, 0.f, 0.f}, .radius = 0.45f, .isSweeping = true
		});
		hitVolume.AddShape({
			.role = HitVolumeRole::HITBOX, .name = "forearm", .groupName = "clawL", .boneName = "LowerArm_L",
			.localCenterB = {1.72f, 0.f, 0.f}, .radius = 0.45f, .isSweeping = true
		});

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
		// find times with the tester: P to pause, <- / -> to step
		animator->AddEvent("attackHand", 1.2f, "HitboxOn");
		animator->AddEvent("attackHand", 1.65f, "HitboxOff");
		animator->AddEvent("Walk", 0.f, "Footstep");
		animator->AddEvent("Walk", 0.64f, "Footstep");


		animator->PlayClip("idle02");

		object.AddComponent<CycleAnimationTester>();
	});
}
