#include "App.h"

#include "AnimatorComponent.h"
#include "GameObject.h"
#include "Material.h"
#include "ModelReader.h"
#include "MonoBehavior.h"
#include "SpriteAnimatorComponent.h"
#include "SpriteRendererComponent.h"
#include "SpriteVertex.h"
#include "TextureCache.h"

App::App(const std::string& cmdLine) : cmdLine(cmdLine),
                                       wnd(800, 600, L"DXPractice"),
                                       renderer(wnd.GetRenderer()), renderSystem(RenderSystem(renderer))
{
}

App::~App()
{
}

int App::Run()
{
	Init();

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
	// cube
	//std::unique_ptr<ModelReader> model = std::make_unique<ModelReader>(renderer, "../../assets/testCube.fbx");

	//MaterialData matData = {};
	//matData.color = DirectX::XMFLOAT3(0.3f, 0.4f, 1.0f);
	//matData.specularIntensity = 0.5f;
	//matData.specularPower = 5.0f;
	//std::unique_ptr<GameObject> cube = std::make_unique<GameObject>(renderer, matData, *model);
	//cube->GetTransform()->SetRotation(rotation);

	//gameObjects.push_back(std::move(cube));

	// sprite
	MeshData quad = MakeSpriteQuad();
	auto sprite = std::make_unique<GameObject>(renderer, quad.vertices, quad.indices, L"SpriteVertexShader.cso",
	                                           L"SpritePixelShader.cso");
	sprite->Init(scriptSystem, animationSystem, renderSystem);

	sprite->GetTransform()->SetScale({6, 6, 1});
	sprite->GetTransform()->SetPosition({0, 0, 3});

	sprite->AddComponent<AnimatorComponent>(renderer);
	sprite->GetComponent<AnimatorComponent>()->AddAnimation("CharIdle", L"../../assets/PlayerCharacter.png",
	                                                        L"../../assets/PlayerCharacter.json");
	sprite->GetComponent<AnimatorComponent>()->SetCurrAnimation("CharIdle");
	gameObjects.push_back(std::move(sprite));

	// light 
	LightData light = {};
	light.lightPos = {0.0f, 3.0f, -5.0f};
	light.ambient = {0.1f, 0.1f, 0.1f};
	light.diffuseColor = {1.0f, 1.0f, 1.0f};
	light.diffuseIntensity = 1.0f;
	light.attConst = 1.0f;
	light.attLin = 0.045f;
	light.attQuad = 0.0075f;

	lightCBuffer = std::make_unique<LightCBuffer>(renderer, light);
	for (auto& gameObject : gameObjects)
	{
	}

	wnd.mouse.EnableRaw();
}

void App::Update(float deltaTime)
{
	//LightData light = {};
	//light.lightPos =
	//{
	//	sinf(angle) * 5,
	//	3,
	//	cosf(angle) * -5
	//};

	//light.ambient = { 1, 0.1f, 0.1f };
	//light.diffuseColor = { 1, 1.0f, 1.0f };
	//light.diffuseIntensity = 1.0f;
	//light.attConst = 0.2f;
	//light.attLin = 0.0045f;
	//light.attQuad = 0.0075f;

	//lightCBuffer->Update(renderer, light);

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

	// systems
	scriptSystem.Update(deltaTime);
	animationSystem.Update(deltaTime);
}

void App::HandleInput(float deltaTime)
{
}

void App::Draw(float deltaTime)
{
	renderer.BeginFrame(0.3f, 1, 0.3f);

	if (lightCBuffer != nullptr)
	{
		lightCBuffer->Bind(renderer);
	}

	renderSystem.Render();
	renderer.EndFrame();
}
