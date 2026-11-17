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
		object.AddComponent<HealthComponent>(1000.f);
	});

	// player
	PrefabRegistry::Instance().RegisterModel("player", "../../assets/models/testCube.fbx", mat, [](GameObject& object)
	{
		object.GetTransform()->SetScale({0.6f, 1.8f, 0.35f});
		object.SetTag(ObjectTag::PLAYER);
		object.AddComponent<PlayerController>();
		object.AddComponent<LocomotionComponent>();
		object.AddComponent<CameraController>();
		object.AddComponent<HealthComponent>(100.f);

		// fallback until data/hitvolumes/player.json is saved from the inspector
		HitVolumeDesc fallback;
		fallback.shapes = {
			{
				.role = HitVolumeRole::HURTBOX, .name = "body",
				.localCenterA = {0.f, -0.55f, 0.f}, .localCenterB = {0.f, 0.55f, 0.f}, .radius = 0.35f
			}
		};
		object.AddComponent<HitVolumeComponent>().LoadFile("../../data/hitvolumes/player.json", fallback);
	});
}
