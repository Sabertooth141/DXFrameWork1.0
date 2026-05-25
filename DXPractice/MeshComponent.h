#pragma once
#include <memory>
#include <vector>

#include "Component.h"
#include "DrawableBase.h"
#include "TransformComponent.h"
#include "Vertex.h"

/**
 * handles vertices, indices, binds VB IB and topology on construct
 * takes component class as interface, drawableBase as base class
 */
class MeshComponent : public Component, public DrawableBase<MeshComponent>
{
public:
	MeshComponent(Renderer& renderer, std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, TransformComponent& transformComp);
};

