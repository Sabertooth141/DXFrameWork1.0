#pragma once
#include "Component.h"
#include "DrawableBase.h"

struct MaterialData;
/**
 * handles materials, shaders, input layouts
 * binds vs, ps, inputLayout and matCBuffer
 */
class MaterialComponent : public Component
{
public:
	MaterialComponent(Renderer& renderer, const MaterialData& matData);

	void Bind(Renderer& renderer);

private:
	static std::vector<std::unique_ptr<Bindable>> staticBinds;
	std::vector<std::unique_ptr<Bindable>> instanceBinds;
};

