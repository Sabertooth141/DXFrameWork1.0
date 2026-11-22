#include "PrefabRegistry.h"

#include "BoxCollider2D.h"
#include "CameraController.h"
#include "CharacterData.h"
#include "CycleAnimationTester.h"
#include "GameObject.h"
#include "HealthComponent.h"
#include "HitVolumeComponent.h"
#include "PhysicsTest.h"
#include "PlayerController.h"
#include "LocomotionComponent.h"
#include "DragonAI.h"

class DragonAI;

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
	PrefabRegistry::Instance().RegisterModel("dragon", "../../assets/models/scifiDragon/Mesh/Sci-Fi Dragon 4Wings Skin 1.fbx", mat, [](GameObject& object)
	{
		ApplyCharacterData(object, DragonData());
		object.SetTag(ObjectTag::ENEMY);
		object.GetComponent<MeshRendererComponent>()->doubleSided = true;
		//object.AddComponent<CycleAnimationTester>();
		object.AddComponent<DragonAI>();

		auto& locomotion = object.AddComponent<LocomotionComponent>();
		locomotion.isPushedOut = false;
		locomotion.bodyRadius = 3.f;

		object.AddComponent<HealthComponent>(1000.f);
	});

	// player
	PrefabRegistry::Instance().RegisterModel("player", "../../assets/models/10e/mesh/violinistBaseMesh.fbx", mat, [](GameObject& object)
	{
		ApplyCharacterData(object, PlayerData());
		object.SetTag(ObjectTag::PLAYER);
		object.GetComponent<MeshRendererComponent>()->doubleSided = true;

		object.AddComponent<PlayerController>();
		auto& locomotion = object.AddComponent<LocomotionComponent>();
		locomotion.bodyCenterHeight = 0.9f; // origin is at her feet

		object.AddComponent<CameraController>();
		object.AddComponent<HealthComponent>(100.f);

		// temporary: N/B/R to look through the clips
		//object.AddComponent<CycleAnimationTester>();
	});
}
