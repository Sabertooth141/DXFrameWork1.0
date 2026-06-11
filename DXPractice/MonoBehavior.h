#pragma once
#include "IComponent.h"

class Mouse;
class Keyboard;

class MonoBehavior : public IComponent
{
public:
	virtual void Awake()
	{
	}

	virtual void Start()
	{
	}

	void Update(float deltaTime) override
	{
	}

	virtual void OnDestroy()
	{
	}

	void SetInput(Keyboard& inKeyboard, Mouse& inMouse);
	bool IsStarted() const { return started; }
	bool IsEnabled() const { return enabled; }
	void SetStarted(const bool inStarted) { started = inStarted; }
	void SetEnabled(const bool inEnabled) { enabled = inEnabled; }
public:
	Keyboard* keyboard = nullptr;
	Mouse* mouse = nullptr;

private:
	bool started = false;
	bool enabled = true;
};
