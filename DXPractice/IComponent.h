#pragma once
#include "HitEvent.h"

struct GameContext;
class GameObject;
class Renderer;
class Collider2D;

class IComponent
{
public:
	virtual ~IComponent() = default;

	virtual void Update(float deltaTime)
	{
	}

	virtual void OnCollisionEnter2D(const GameObject& other)
	{
	}

	virtual void OnCollisionStay2D(const GameObject& other)
	{
	}

	virtual void OnCollisionExit2D(const GameObject& other)
	{
	}

	virtual void OnHitDealt(const HitEvent& event)
	{
	}

	virtual void OnHitTaken(const HitEvent& event)
	{
	}

	virtual void OnAttach(GameContext& context)
	{
	}

public:
	GameObject* owner = nullptr;
};
