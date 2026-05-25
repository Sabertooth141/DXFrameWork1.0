#include "MeshComponent.h"

#include "Topology.h"
#include "TransformCBuffer.h"
#include "VertexBuffer.h"

MeshComponent::MeshComponent(Renderer& renderer, std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, TransformComponent& transformComp)
{
	AddBind(std::make_unique<VertexBuffer>(renderer, vertices));
	AddIndexBuffer(std::make_unique<IndexBuffer>(renderer, indices));
	AddBind(std::make_unique<TransformCBuffer>(renderer, transformComp));

	if (!IsStaticInit())
	{
		AddStaticBind(std::make_unique<Topology>(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST));
	}
}
