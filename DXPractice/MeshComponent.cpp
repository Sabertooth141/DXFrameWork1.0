#include "MeshComponent.h"

#include "Topology.h"
#include "VertexBuffer.h"

MeshComponent::MeshComponent(Renderer& renderer, std::vector<Vertex>& vertices, std::vector<unsigned int>& indices)
{
	AddBind(std::make_unique<VertexBuffer>(renderer, vertices));
	AddIndexBuffer(std::make_unique<IndexBuffer>(renderer, indices));

	if (!IsStaticInit())
	{
		AddStaticBind(std::make_unique<Topology>(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST));
	}
}
