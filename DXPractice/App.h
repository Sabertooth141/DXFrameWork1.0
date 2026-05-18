#pragma once
#include "Window.h"
#include <string>

#include "LightCBuffer.h"
#include "Timer.h"

class Model;

class App
{
public:
	App(const std::string& cmdLine = "");
	~App();

	int Run();
private:
	void Init();
	void Update(float deltaTime);
	void HandleInput(float deltaTime);
	void Draw(float deltaTime);
private:
	std::string cmdLine;

	Window wnd;
	Renderer renderer;
	Timer timer;
	std::unique_ptr<Model> model;
	std::unique_ptr<LightCBuffer> lightCBuffer;
};

