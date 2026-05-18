#include "Mesh.h"

#include "InputLayout.h"
#include "PixelShader.h"
#include "Topology.h"
#include "TransformCBuffer.h"
#include "VertexBuffer.h"
#include "VertexShader.h"

Mesh::Mesh(Renderer& renderer, const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices)
{
	AddBind(std::make_unique<VertexBuffer>(renderer, vertices));
	AddIndexBuffer(std::make_unique<IndexBuffer>(renderer, indices));

	auto vShader = std::make_unique<VertexShader>(renderer, L"VertexShader.cso");
	auto vsByteCode = vShader->GetByteCode();

	AddBind(std::move(vShader));
	AddBind(std::make_unique<PixelShader>(renderer, L"PixelShader.cso"));

	const std::vector<D3D11_INPUT_ELEMENT_DESC> layout =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	AddBind(std::make_unique<InputLayout>(renderer, layout, vsByteCode));
	AddBind(std::make_unique<Topology>(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST));
	AddBind(std::make_unique<TransformCBuffer>(renderer, *this));
}

void Mesh::Update(float deltaTime)
{
}

DirectX::XMMATRIX Mesh::GetTransformMatrix()
{
	return DirectX::XMMatrixTranslation(position.x, position.y, position.z);
}
