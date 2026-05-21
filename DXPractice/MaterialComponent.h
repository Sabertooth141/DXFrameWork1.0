#pragma once
#include "Component.h"
#include "DrawableBase.h"

struct MaterialData;
/**
 * handles materials, shaders, input layouts
 * binds vs, ps, inputLayout and matCBuffer
 */
class MaterialComponent : public Component, public DrawableBase<MaterialComponent>
{
public:
	MaterialComponent(Renderer& renderer, MaterialData& matData);
	void Draw(Renderer& renderer) override;
};

