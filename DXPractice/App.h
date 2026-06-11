#pragma once
#include "Window.h"
#include <string>

#include "AnimationSystem.h"
#include "LightCBuffer.h"
#include "RenderSystem.h"
#include "ScriptSystem.h"
#include "Timer.h"

class GameObject;
class ModelReader;

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
	DirectX::XMFLOAT3 rotation = {};

	std::string cmdLine;

	Window wnd;
	Renderer renderer;
	Timer timer;
	ScriptSystem scriptSystem;
	AnimationSystem animationSystem;
	RenderSystem renderSystem;

	std::unique_ptr<LightCBuffer> lightCBuffer;

	std::vector<std::unique_ptr<GameObject>> gameObjects;

	float sensitivity = 0.004f;
};

