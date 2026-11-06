#include "PrefabRegistry.h"

#include <memory>

#include "AnimationClip.h"
#include "AnimationClipCache.h"
#include "BoxCollider2D.h"
#include "CycleAnimationTester.h"
#include "GameObject.h"
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
			table->SetFade("idle01", "walk", 0.3f);
			table->SetFade("walk", "idle01", 0.3f);
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
	PrefabRegistry::Instance().RegisterModel("dragon", "../../assets/models/dragon/DragonUsurperMesh.fbx", mat,
	                                         [](GameObject& object)
	                                         {
		                                         object.GetTransform()->SetScale(0.01f);

		                                         auto* animator = object.GetComponent<SkeletalAnimatorComponent>();
		                                         const std::string animDir = "../../assets/models/dragon/animations/";

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
		                                         animator->PlayClip("idle02");

		                                         object.AddComponent<CycleAnimationTester>();
	                                         });
}
