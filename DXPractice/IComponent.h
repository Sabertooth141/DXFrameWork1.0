#pragma once

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

	virtual void OnCollisionEnter2D(const Collider2D& other)
	{
	}

	virtual void OnCollisionExit2D(const Collider2D& other)
	{
	}

public:
	GameObject* owner = nullptr;
};
