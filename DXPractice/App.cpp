#include "App.h"

App::App(const std::string& cmdLine) : cmdLine(cmdLine),
                                       wnd(800, 600, L"DXPractice"),
                                       renderer(wnd.GetRenderer()),
                                       box(wnd.GetRenderer())
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
}

void App::Update(float deltaTime)
{
	box.Update(deltaTime);
}

void App::HandleInput(float deltaTime)
{
}

void App::Draw(float deltaTime)
{
	renderer.BeginFrame(1, 1, 1);
	box.Draw(renderer);
	renderer.EndFrame();
}
