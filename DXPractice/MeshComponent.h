#pragma once
#include <memory>
#include <vector>

#include "IComponent.h"
#include "DrawableBase.h"
#include "MeshData.h"
#include "TransformComponent.h"

/**
 * handles vertices, indices, binds VB IB and topology on construct
 * takes component class as interface, drawableBase as base class
 */
class MeshComponent : public IComponent, public DrawableBase<MeshComponent>
{
public:
	MeshComponent(Renderer& renderer, const MeshData& meshData, TransformComponent& transformComp);
	MeshComponent(Renderer& renderer, const SkinnedMeshData& meshData, TransformComponent& transformComp);
	void Update(float deltaTime) override;

private:
	void Init(Renderer& renderer, const std::vector<unsigned int>& indices, TransformComponent& transformComp);
};

