#include "App.h"

#include "Model.h"

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
	model = std::make_unique<Model>(renderer, "../../assets/testCube.obj");

	LightData light = {};
	light.lightPos = {0.0f, 3.0f, -5.0f};
	light.ambient = {0.1f, 0.1f, 0.1f};
	light.diffuseColor = {1.0f, 1.0f, 1.0f};
	light.diffuseIntensity = 1.0f;
	light.attConst = 1.0f;
	light.attLin = 0.045f;
	light.attQuad = 0.0075f;

	lightCBuffer = std::make_unique<LightCBuffer>(renderer, light);
}

void App::Update(float deltaTime)
{
	angle += deltaTime;

	LightData light = {};
	light.lightPos =
	{
		sinf(angle) * 5,
		3,
		cosf(angle) * -5
	};

	light.ambient = { 0.1f, 0.1f, 0.1f };
	light.diffuseColor = { 1.0f, 1.0f, 1.0f };
	light.diffuseIntensity = 1.0f;
	light.attConst = 1.0f;
	light.attLin = 0.045f;
	light.attQuad = 0.0075f;

	lightCBuffer->Update(renderer, light);

}

void App::HandleInput(float deltaTime)
{
}

void App::Draw(float deltaTime)
{
	renderer.BeginFrame(.3, .3, .3);

	lightCBuffer->Bind(renderer);
	model->Draw(renderer);
	renderer.EndFrame();
}
