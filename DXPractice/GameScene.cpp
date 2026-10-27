#include "GameScene.h"

#include <format>

#include "Material.h"
#include "SceneManager.h"
#include "TextRenderer.h"

void GameScene::OnEnter()
{
	Scene::OnEnter();

	// 3d test
	MaterialData mat{};
	mat.specularIntensity = 0.5f;
	mat.specularPower = 2.f;

	GameObject* testCube = Add3DObject("../../assets/models/testCube.fbx", mat);
	testCube->GetTransform()->SetPosition({ 0.f, 0.f, 0.f });
	testCube->SetName("cube");

	// light test
	GameObject* testLight = AddEmptyObject();
	auto& lightComp = testLight->AddComponent<LightComponent>();
	lightComp.color = { 1.f, 0.8f, 0.6f };
	testLight->GetTransform()->SetPosition({ 0.f, 5.f, -5.f });
	testLight->SetName("light");
}

void GameScene::OnUpdate(float deltaTime)
{
	
}

void GameScene::OnDrawUI(TextRenderer& text)
{
	Scene::OnDrawUI(text);
}
