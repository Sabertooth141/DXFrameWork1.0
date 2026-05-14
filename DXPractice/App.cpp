#include "App.h"

App::App(const std::string& cmdLine) : cmdLine(cmdLine), wnd(800, 600, L"DXPractice")
{
}

App::~App()
{
}

int App::Start()
{
	while (true)
	{
		if (const auto exitCode = Window::ProcessMessages())
		{
			return *exitCode;
		}

		// TODO: set deltatime
		const float deltaTime = 1;
		HandleInput(deltaTime);
		Update(deltaTime);
	}
}

void App::Update(float deltaTime)
{
}

void App::HandleInput(float deltaTime)
{

}
