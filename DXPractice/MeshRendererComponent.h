#pragma once
#include "IComponent.h"

class MeshRendererComponent : public IComponent
{
public:
	void Render(Renderer& renderer);

public:
	bool isEnabled = true;
	bool castShadows = true;
};
