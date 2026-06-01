#include "App.h"

#include "GameObject.h"
#include "Material.h"
#include "ModelReader.h"

App::App(const std::string& cmdLine) : cmdLine(cmdLine),
                                       wnd(800, 600, L"DXPractice"),
                                       renderer(wnd.GetRenderer())
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
	std::unique_ptr<ModelReader> model = std::make_unique<ModelReader>(renderer, "../../assets/testCube.obj");

	MaterialData matData = {};
	matData.color = DirectX::XMFLOAT3(0.3f, 0.4f, 1.0f);
	matData.specularIntensity = 0.5f;
	matData.specularPower = 5.0f;
	std::unique_ptr<GameObject> cube = std::make_unique<GameObject>(renderer, matData, *model);
	cube->GetTransform()->SetRotation(rotation);

	gameObjects.push_back(std::move(cube));

	// sprite
	std::unique_ptr<GameObject> sprite = std::make_unique<GameObject>(renderer);
	sprite->AddComponent<MaterialComponent>(renderer, MaterialData{}, L"SpriteVertexShader.cso", L"SpritePixelShader.cso");
	sprite->GetTransform()->SetPosition(DirectX::XMFLOAT3(2, 0, 0));

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

	gameObjects[0]->GetTransform()->SetRotation(rotation);
	for (const auto& gameObject : gameObjects)
	{
		gameObject->Update(deltaTime);
	}
}

void App::HandleInput(float deltaTime)
{
}

void App::Draw(float deltaTime)
{
	renderer.BeginFrame(0.3f, 1, 0.3f);

	lightCBuffer->Bind(renderer);
	for (const auto& gameObject : gameObjects)
	{
		gameObject->Draw(renderer);
	}

	renderer.EndFrame();
}
