#pragma once
#include <memory>
#include <vector>

#include "Component.h"
#include "DrawableBase.h"

struct Vertex;
/**
 * handles vertices, indices, binds VB IB and topology on construct
 * takes component class as interface, drawableBase as base class
 */
class MeshComponent : public Component, public DrawableBase<MeshComponent>
{
public:
	MeshComponent(Renderer& renderer, std::vector<Vertex>& vertices, std::vector<unsigned int>& indices);
};

