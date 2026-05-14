#pragma once
#include "Window.h"
#include <string>

class App
{
public:
	App(const std::string& cmdLine = "");
	~App();

	int Start();
private:
	void Update(float deltaTime);
	void HandleInput(float deltaTime);
private:
	std::string cmdLine;
	Window wnd;
};

