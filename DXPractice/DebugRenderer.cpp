#include "DebugRenderer.h"

#include "InputLayout.h"
#include "PixelShader.h"
#include "VertexShader.h"

namespace
{
	struct DebugTransforms
	{
		DirectX::XMMATRIX model;
		DirectX::XMMATRIX view;
		DirectX::XMMATRIX projection;
	};
}

DebugRenderer::DebugRenderer(Renderer& renderer)
{
	vertexShader = std::make_unique<VertexShader>(renderer, L"ColorVertexShader.cso");
	pixelShader = std::make_unique<PixelShader>(renderer, L"ColorPixelShader.cso");

	std::vector<D3D11_INPUT_ELEMENT_DESC> layout =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	inputLayout = std::make_unique<InputLayout>(renderer, layout, vertexShader->GetByteCode());

	DebugTransforms init{};
	D3D11_BUFFER_DESC cbd = {};
	cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbd.Usage = D3D11_USAGE_DYNAMIC;
	cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbd.ByteWidth = sizeof(DebugTransforms);
	D3D11_SUBRESOURCE_DATA csd = {};
	csd.pSysMem = &init;
	renderer.pDevice->CreateBuffer(&cbd, &csd, &transformCBuffer);

	vertices.reserve(256);
}

DebugRenderer::~DebugRenderer() = default;

void DebugRenderer::Begin()
{
	vertices.clear();
	vertices3D.clear();
}

void DebugRenderer::DrawLine(DirectX::XMFLOAT2 start, DirectX::XMFLOAT2 end, DirectX::XMFLOAT4 color)
{
	vertices.push_back({{start.x, start.y, debugDepth}, color});
	vertices.push_back({{end.x, end.y, debugDepth}, color});
}

void DebugRenderer::DrawBox(const std::array<DirectX::XMFLOAT2, 4>& corners, DirectX::XMFLOAT4 color)
{
	DrawLine(corners[0], corners[1], color);
	DrawLine(corners[1], corners[2], color);
	DrawLine(corners[2], corners[3], color);
	DrawLine(corners[3], corners[0], color);
}

void DebugRenderer::DrawCross(DirectX::XMFLOAT2 center, const float size, DirectX::XMFLOAT4 color)
{
	DrawLine({center.x - size, center.y}, {center.x + size, center.y}, color);
	DrawLine({center.x, center.y - size}, {center.x, center.y + size}, color);
}

void DebugRenderer::Flush(Renderer& renderer)
{
	DrawBatch(renderer, vertices, renderer.GetView(), renderer.GetProj());
}

void DebugRenderer::Flush3D(Renderer& renderer, DirectX::FXMMATRIX view)
{
	if (vertices3D.empty())
	{
		return;
	}

	renderer.SetDepthEnabled(false);
	DrawBatch(renderer, vertices3D, view, renderer.Get3DProj());
}

void DebugRenderer::DrawBatch(Renderer& renderer, const std::vector<DebugVertex>& vertices, DirectX::FXMMATRIX view, DirectX::CXMMATRIX proj)
{
	if (vertices.empty())
	{
		return;
	}

	auto* ctx = renderer.pContext.Get();

	EnsureCapacity(renderer, vertices.size());
	{
		D3D11_MAPPED_SUBRESOURCE msr;
		ctx->Map(vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
		memcpy(msr.pData, vertices.data(), sizeof(DebugVertex) * vertices.size());
		ctx->Unmap(vertexBuffer.Get(), 0);
	}

	{
		DebugTransforms t;
		t.model = DirectX::XMMatrixIdentity();
		t.view = DirectX::XMMatrixTranspose(view);
		t.projection = DirectX::XMMatrixTranspose(proj); // active proj

		D3D11_MAPPED_SUBRESOURCE msr;
		ctx->Map(transformCBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
		memcpy(msr.pData, &t, sizeof(t));
		ctx->Unmap(transformCBuffer.Get(), 0);
	}

	// pipeline
	vertexShader->Bind(renderer);
	pixelShader->Bind(renderer);
	inputLayout->Bind(renderer);
	ctx->VSSetConstantBuffers(0, 1, transformCBuffer.GetAddressOf());
	ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	const UINT stride = sizeof(DebugVertex);
	const UINT offset = 0;
	ctx->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);

	// draw at the end 
	ctx->Draw(static_cast<UINT>(vertices.size()), 0);
}

void DebugRenderer::DrawLine3D(DirectX::XMFLOAT3 start, DirectX::XMFLOAT3 end, DirectX::XMFLOAT4 color)
{
	vertices3D.push_back({start, color});
	vertices3D.push_back({ end, color });
}

void DebugRenderer::DrawSphere3D(DirectX::XMFLOAT3 center, float radius, DirectX::XMFLOAT4 color)
{
	// for matrix calculations
	using namespace DirectX;

	const XMVECTOR centerPos = XMLoadFloat3(&center);
	const XMVECTOR x = XMVectorSet(1.f, 0.f, 0.f, 0.f);
	const XMVECTOR y = XMVectorSet(0.f, 1.f, 0.f, 0.f);
	const XMVECTOR z = XMVectorSet(0.f, 0.f, 1.f, 0.f);

	DrawArc3D(centerPos, x, y, radius, 0.f, XM_2PI, circleSegments, color);
	DrawArc3D(centerPos, y, z, radius, 0.f, XM_2PI, circleSegments, color);
	DrawArc3D(centerPos, z, x, radius, 0.f, XM_2PI, circleSegments, color);
}

void DebugRenderer::DrawCapsule3D(DirectX::XMFLOAT3 centerA, DirectX::XMFLOAT3 centerB, float radius, DirectX::XMFLOAT4 color)
{
}

void DebugRenderer::DrawArc3D(DirectX::FXMVECTOR center, DirectX::FXMVECTOR axisX, DirectX::FXMVECTOR axisY, float radius, float startRad, float endRad, int segments, DirectX::XMFLOAT4 color)
{
	// for matrix calculations
	using namespace DirectX;

	const float step = (endRad - startRad) / static_cast<float>(segments);

	DirectX::XMFLOAT3 prevVertex;
	DirectX::XMStoreFloat3(&prevVertex, center + (axisX * cosf(startRad) + axisY * sinf(startRad)) * radius);
}

void DebugRenderer::EnsureCapacity(const Renderer& renderer, const size_t vertexCount)
{
	if (vertexCount <= vertexBufferCapacity && vertexBuffer)
	{
		return;
	}

	vertexBufferCapacity = vertexCount * 2 + 64;

	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bd.ByteWidth = static_cast<UINT>(sizeof(DebugVertex) * vertexBufferCapacity);
	bd.StructureByteStride = sizeof(DebugVertex);

	vertexBuffer.Reset();
	renderer.pDevice->CreateBuffer(&bd, nullptr, &vertexBuffer);
}
