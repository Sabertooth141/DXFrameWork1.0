#include "Box.h"

#include "ConstantBuffer.h"
#include "InputLayout.h"
#include "PixelShader.h"
#include "Topology.h"
#include "TransformCBuffer.h"
#include "VertexBuffer.h"
#include "VertexShader.h"

Box::Box(Renderer& renderer)
{
	struct Vertex
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT3 color;
	};

	std::vector<Vertex> vertices =
	{
		{{-1, -1, -1}, {1, 0, 0}},		// bottom left front 
		{{1, -1, -1}, {0, 1, 0}},		// bottom right front
		{{-1, 1, -1}, {0, 0, 1}},		// top left front
		{{1, 1, -1}, {1, 1, 0}},		// top right front
		{{-1, -1, 1}, {1, 0, 1}},		// bottom left back
		{{1, -1, 1}, {0, 1, 1}},		// bottom right back
		{{-1, 1, 1}, {1, 1, 1}},		// top left back
		{{1, 1, 1}, {0, 0, 0}},			// top right back
	};


	const std::vector<unsigned short> indices =
	{
		0, 2, 1,  1, 2, 3,
		1, 3, 5,  5, 3, 7,
		5, 7, 4,  4, 7, 6,
		4, 6, 0,  0, 6, 2,
		2, 6, 3,  3, 6, 7,
		4, 0, 5,  5, 0, 1
	};

	AddBind(std::make_unique<VertexBuffer>(renderer, vertices));
	AddIndexBuffer(std::make_unique<IndexBuffer>(renderer, indices));

	const std::vector<D3D11_INPUT_ELEMENT_DESC> layout =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, color), D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	std::unique_ptr<VertexShader> vShader = std::make_unique<VertexShader>(renderer, L"VertexShader.cso");
	ID3DBlob* vsByteCode = vShader->GetByteCode();

	AddBind(std::move(vShader));
	AddBind(std::make_unique<PixelShader>(renderer, L"PixelShader.cso"));

	AddBind(std::make_unique<InputLayout>(renderer, layout, vsByteCode));
	AddBind(std::make_unique<Topology>(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST));
	AddBind(std::make_unique<TransformCBuffer>(renderer, *this));
}

DirectX::XMMATRIX Box::GetTransformMatrix()
{
	return DirectX::XMMatrixRotationY(angle) * DirectX::XMMatrixTranslation(0, 0, 4);
}

void Box::Update(float deltaTime)
{
	angle += deltaTime;
}
