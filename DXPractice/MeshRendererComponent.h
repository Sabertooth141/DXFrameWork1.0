#pragma once
#include "IComponent.h"

class MeshRendererComponent : public IComponent
{
public:
	void Render(Renderer& renderer);

	void OnAttach(GameContext& context) override;

public:
	bool isEnabled = true;
	bool castShadows = true;
};
