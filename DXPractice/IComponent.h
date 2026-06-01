#pragma once
class Renderer;

class IComponent
{
public:
	virtual ~IComponent() = default;

	virtual void Update(float deltaTime)
	{
	}
};
