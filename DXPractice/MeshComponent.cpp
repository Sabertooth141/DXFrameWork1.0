#include "MeshComponent.h"

#include "Topology.h"
#include "TransformCBuffer.h"
#include "VertexBuffer.h"

MeshComponent::MeshComponent(Renderer& renderer, const MeshData& meshData, TransformComponent& transformComp)
{
	AddBind(std::make_unique<VertexBuffer>(renderer, meshData.vertices));
	Init(renderer, meshData.indices, transformComp);
}

MeshComponent::MeshComponent(Renderer& renderer, const SkinnedMeshData& meshData, TransformComponent& transformComp)
{
	AddBind(std::make_unique<VertexBuffer>(renderer, meshData.vertices));
	Init(renderer, meshData.indices, transformComp);
}

void MeshComponent::Init(Renderer& renderer, const std::vector<unsigned int>& indices,
	TransformComponent& transformComp)
{
	AddIndexBuffer(std::make_unique<IndexBuffer>(renderer, indices));
	AddBind(std::make_unique<TransformCBuffer>(renderer, transformComp));

	if (!IsStaticInit())
	{
		AddStaticBind(std::make_unique<Topology>(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST));
	}
}

void MeshComponent::Update(float deltaTime)
{
	IComponent::Update(deltaTime);
}
