#include "App.h"

#include "AnimatorComponent.h"
#include "CollisionTests.h"
#include "GameObject.h"
#include "Material.h"
#include "ModelReader.h"
#include "MonoBehavior.h"
#include "PhysicsTest.h"
#include "PlayerController.h"
#include "PrefabRegistry.h"
#include "SpriteAnimatorComponent.h"
#include "SpriteRendererComponent.h"
#include "SpriteVertex.h"
#include "TextureCache.h"
#include "WindowSettings.h"

App::App(const std::string& cmdLine) : cmdLine(cmdLine),
                                       wnd(WIN_WIDTH, WIN_HEIGHT, L"DXPractice"),
                                       renderer(wnd.GetRenderer()), debugRenderer(renderer),
                                       renderSystem(RenderSystem(renderer)),
                                       gameContext{
	                                       renderer, physicsSystem, scriptSystem, animationSystem, renderSystem
                                       },
                                       scene(gameContext)
{
}

App::~App()
{
}

int App::Run()
{
	Init();

	timer.Mark();
	while (true)
	{
		if (const auto exitCode = Window::ProcessMessages())
		{
			return *exitCode;
		}

		const float deltaTime = timer.Mark();
		HandleInput(deltaTime);
		Update(deltaTime);
		Draw(deltaTime);
	}
}

void App::Init()
{
	RegisterPrefabs();
	RegisterCollisionTests();
	// cube
	//std::unique_ptr<ModelReader> model = std::make_unique<ModelReader>(renderer, "../../assets/testCube.fbx");

	//MaterialData matData = {};
	//matData.color = DirectX::XMFLOAT3(0.3f, 0.4f, 1.0f);
	//matData.specularIntensity = 0.5f;
	//matData.specularPower = 5.0f;
	//std::unique_ptr<GameObject> cube = std::make_unique<GameObject>(renderer, matData, *model);
	//cube->GetTransform()->SetRotation(rotation);

	//gameObjects.push_back(std::move(cube));

	// player
	MeshData quad = MakeSpriteQuad();
	auto sprite = scene.Add2DObject();

	sprite->GetTransform()->SetPosition({-128, -100, 1});

	// physics
	Rigidbody2DComponent* rb = &sprite->AddComponent<Rigidbody2DComponent>(*sprite->GetTransform(), 1.0f);
	BoxCollider2D* col = &sprite->AddComponent<BoxCollider2D>(DirectX::XMFLOAT2(1, 1), DirectX::XMFLOAT2(0, 0), true,
	                                                          *sprite->GetTransform());
	rb->SetFreezeRotation(true);
	rb->SetGravity(0.f);

	// animation
	sprite->AddComponent<AnimatorComponent>(renderer);
	sprite->GetComponent<AnimatorComponent>()->SetRenderLayer(RenderLayer::Player);
	sprite->GetComponent<AnimatorComponent>()->SetSortOrder(0);
	sprite->GetComponent<AnimatorComponent>()->AddAnimation("CharIdle", L"../../assets/PlayerCharacter.png",
	                                                        L"../../assets/PlayerCharacter.json");
	sprite->GetComponent<AnimatorComponent>()->AddAnimation("CharMove", L"../../assets/PlayerCharacterMove.png",
	                                                        L"../../assets/PlayerCharacterMove.json");
	sprite->GetComponent<AnimatorComponent>()->SetCurrAnimation("CharIdle");

	// script
	sprite->AddComponent<PlayerController>();
	sprite->GetComponent<PlayerController>()->SetInput(wnd.keyboard, wnd.mouse);


	//// layer test
	//auto backGround = std::make_unique<GameObject>(quad.vertices, quad.indices, gameContext);

	//backGround->GetTransform()->SetPosition({0, 0, 1});

	//backGround->AddComponent<AnimatorComponent>(renderer);
	//backGround->GetComponent<AnimatorComponent>()->SetRenderLayer(RenderLayer::BackGround);
	//backGround->GetComponent<AnimatorComponent>()->SetSortOrder(0);
	//backGround->GetComponent<AnimatorComponent>()->SetStatic(L"../../assets/bgTest.jpg");

	// ground
	// physics test
	MeshData groundQuad = MakeSpriteQuad();
	auto groundObj = scene.Add2DObject();

	groundObj->GetTransform()->SetPosition({0, -300, 1});

	groundObj->AddComponent<AnimatorComponent>(renderer);
	groundObj->GetComponent<AnimatorComponent>()->SetRenderLayer(RenderLayer::Default);
	groundObj->GetComponent<AnimatorComponent>()->SetSortOrder(0);
	groundObj->GetComponent<AnimatorComponent>()->SetStatic(L"../../assets/bgTest.jpg");
	groundObj->GetTransform()->SetScale({1000, 100, 1});

	groundObj->AddComponent<PhysicsTest>();

	// physics
	Rigidbody2DComponent* groundRb = &groundObj->AddComponent<Rigidbody2DComponent>(*groundObj->GetTransform(), 1.0f);
	BoxCollider2D* groundCol = &groundObj->AddComponent<BoxCollider2D>(DirectX::XMFLOAT2(1, 1), DirectX::XMFLOAT2(0, 0),
	                                                                   false, *groundObj->GetTransform());
	groundRb->SetIsStatic(true);

	auto block = scene.Instantiate("block", DirectX::XMFLOAT3(200, 200, 1));
	scene.Instantiate("block", DirectX::XMFLOAT3(120, 0, 1));
	//block->GetComponent<Rigidbody2DComponent>()->SetFreezeRotation(true);

	wnd.mouse.EnableRaw();
}

void App::Update(float deltaTime)
{
	HandleInput(deltaTime);

	scene.Update(deltaTime);

	// systems
	scriptSystem.Update(deltaTime);
	physicsSystem.Update(deltaTime);
	animationSystem.Update(deltaTime);
	scene.FlushPending();
}

void App::HandleInput(float deltaTime)
{
	if (wnd.mouse.LeftPressed())
	{
		if (const std::optional<Mouse::RawDelta> delta = wnd.mouse.readRawDelta())
		{
			//char buf[64];
			//sprintf_s(buf, "raw x: %f, y: %f \n", delta.value().x, delta.value().y);
			//OutputDebugStringA(buf);
			rotation.x += delta.value().y * sensitivity;
			rotation.y += delta.value().x * sensitivity;
		}
	}
}

void App::Draw(float deltaTime)
{
	renderer.BeginFrame(0, 0, 0);

	if (lightCBuffer != nullptr)
	{
		lightCBuffer->Bind(renderer);
	}

	renderSystem.Render();
	debugRenderer.Begin();
	for (auto& go : scene.GetObjects())
		if (auto* col = go->GetComponent<BoxCollider2D>())
			debugRenderer.DrawBox(col->GetWorldOBB().GetCorners(), {0, 1, 0, 1});
	debugRenderer.Flush(renderer);
	renderer.EndFrame();
}
