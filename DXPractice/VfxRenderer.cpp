#include "VfxRenderer.h"

#include <array>

#include "DebugRenderer.h"
#include "InputLayout.h"
#include "PixelShader.h"
#include "VertexShader.h"

namespace
{
	struct VfxTransforms
	{
		DirectX::XMMATRIX model;
		DirectX::XMMATRIX view;
		DirectX::XMMATRIX proj;
	};

	void MakeBasis(DirectX::FXMVECTOR dir, DirectX::XMVECTOR& outU, DirectX::XMVECTOR& outV)
	{
		using namespace DirectX;
		XMVECTOR helper;
		if (fabsf(XMVectorGetY(dir)) < 0.99f)
		{
			helper = XMVectorSet(0.f, 1.f, 0.f, 0.f);
		}
		else
		{
			helper = XMVectorSet(1.f, 0.f, 0.f, 0.f);
		}

		outU = XMVector3Normalize(XMVector3Cross(helper, dir));
		outV = XMVector3Cross(dir, outU);
	}
}

VfxRenderer::VfxRenderer(Renderer& renderer)
{
	vertexShader = std::make_unique<VertexShader>(renderer, L"ColorVertexShader.cso");
	pixelShader = std::make_unique<PixelShader>(renderer, L"ColorPixelShader.cso");

	std::vector<D3D11_INPUT_ELEMENT_DESC> layout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	inputLayout = std::make_unique<InputLayout>(renderer, layout, vertexShader->GetByteCode());

	ID3D11Device* device = renderer.GetDevice().Get();

	VfxTransforms init{};
	D3D11_BUFFER_DESC cbd = {};
	cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbd.Usage = D3D11_USAGE_DYNAMIC;
	cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbd.ByteWidth = sizeof(VfxTransforms);
	D3D11_SUBRESOURCE_DATA csd = {};
	csd.pSysMem = &init;
	device->CreateBuffer(&cbd, &csd, &transformCBuffer);

	// additive: color = src * srcAlpha + dest. Only adds light, so draw order never matters.
	// alpha channel: keep the back buffer's alpha as it is
	D3D11_BLEND_DESC bd = {};
	bd.RenderTarget[0].BlendEnable = TRUE;
	bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
	bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
	bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	device->CreateBlendState(&bd, &blendAdditive);

	// test against the opaque scene, but never write: glow shapes must not hide each other
	D3D11_DEPTH_STENCIL_DESC dsd = {};
	dsd.DepthEnable = TRUE;
	dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dsd.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	device->CreateDepthStencilState(&dsd, &depthTestNoWrite);

	// both faces: CPU-built shapes cannot vanish from a winding mistake
	CD3D11_RASTERIZER_DESC rsd(D3D11_DEFAULT);
	rsd.CullMode = D3D11_CULL_NONE;
	device->CreateRasterizerState(&rsd, &cullNone);

	vertices.reserve(4096);
} 

VfxRenderer::~VfxRenderer() = default;

void VfxRenderer::Begin()
{
	vertices.clear();
}

void VfxRenderer::DrawCapsule(DirectX::XMFLOAT3 centerA, DirectX::XMFLOAT3 centerB, float radius, DirectX::XMFLOAT4 color)
{
	using namespace DirectX;

	const XMVECTOR posA = XMLoadFloat3(&centerA);
	const XMVECTOR posB = XMLoadFloat3(&centerB);
	const XMVECTOR axis = posB - posA;
	const float length = XMVectorGetX(XMVector3Length(axis));

	// no length = sphere
	const XMVECTOR dir = length > 1e-5f ? axis / length : XMVectorSet(0.f, 1.f, 0.f, 0.f);
	XMVECTOR uDir, vDir;
	MakeBasis(dir, uDir, vDir);


	// rings from A's pole to B's pole. The two equator rings form the cylinder between them.
	constexpr int ringsPerCap = capsuleCapRings + 1;
	constexpr int ringCount = ringsPerCap * 2;
	std::array<XMFLOAT3, ringCount* capsuleSegments> points;

	for (int ring = 0; ring < ringCount; ring++)
	{
		const bool isCapB = ring >= ringsPerCap;
		const int latStep = isCapB ? ring - ringsPerCap : ringsPerCap - 1 - ring; // 0 = equator
		const float lat = XM_PIDIV2 * static_cast<float>(latStep) / static_cast<float>(capsuleCapRings);

		const XMVECTOR capCenter = isCapB ? posB : posA;
		const XMVECTOR capDir = isCapB ? dir : -dir;
		const XMVECTOR ringCenter = capCenter + capDir * (sinf(lat) * radius);
		const float ringRadius = cosf(lat) * radius;

		for (int seg = 0; seg < capsuleSegments; seg++)
		{
			const float angle = XM_2PI * static_cast<float>(seg) / static_cast<float>(capsuleSegments);
			XMStoreFloat3(&points[ring * capsuleSegments + seg],
				ringCenter + (uDir * cosf(angle) + vDir * sinf(angle)) * ringRadius);
		}
	}

	// stitch neighbouring rings (pole rings collapse to a point: those triangles are degenerate and skipped by the GPU)
	for (int ring = 0; ring + 1 < ringCount; ring++)
	{
		for (int seg = 0; seg < capsuleSegments; seg++)
		{
			const int nextSeg = (seg + 1) % capsuleSegments;
			AddQuad(points[ring * capsuleSegments + seg],
				points[ring * capsuleSegments + nextSeg],
				points[(ring + 1) * capsuleSegments + nextSeg],
				points[(ring + 1) * capsuleSegments + seg],
				color);
		}
	}
}

void VfxRenderer::DrawGroundQuad(DirectX::XMFLOAT3 center, float yaw, float halfWidth, float halfLength, DirectX::XMFLOAT4 color)
{
	// forward = sin, cos | right = cos, -sin
	const float fx = sinf(yaw) * halfLength;
	const float fz = cosf(yaw) * halfLength;
	const float rx = cosf(yaw) * halfWidth;
	const float rz = -sinf(yaw) * halfWidth;

	const DirectX::XMFLOAT3 backLeft{ center.x - fx - rx, center.y, center.z - fz - rz };
	const DirectX::XMFLOAT3 frontLeft{ center.x + fx - rx, center.y, center.z + fz - rz };
	const DirectX::XMFLOAT3 frontRight{ center.x + fx + rx, center.y, center.z + fz + rz };
	const DirectX::XMFLOAT3 backRight{ center.x - fx + rx, center.y, center.z - fz + rz };
	AddQuad(backLeft, frontLeft, frontRight, backRight, color);
}

// quad with very thin width
void VfxRenderer::DrawGroundLine(DirectX::XMFLOAT3 start, float yaw, float length, float width, DirectX::XMFLOAT4 color)
{
	const float halfLength = length * 0.5f;
	const DirectX::XMFLOAT3 center{ start.x + sinf(yaw) * halfLength, start.y, start.z + cosf(yaw) * halfLength };
	DrawGroundQuad(center, yaw, width * 0.5f, halfLength, color);
}

void VfxRenderer::DrawGroundFan(DirectX::XMFLOAT3 apex, float yaw, float radius, float halfAngleRad, DirectX::XMFLOAT4 color, int segments)
{
	const float startAngle = yaw - halfAngleRad;
	const float step = halfAngleRad * 2.f / static_cast<float>(segments);

	DirectX::XMFLOAT3 prev{ apex.x + sinf(startAngle) * radius, apex.y, apex.z + cosf(startAngle) * radius };
	for (int i = 1; i <= segments; i++)
	{
		const float angle = startAngle + step * static_cast<float>(i);
		const DirectX::XMFLOAT3 curr{ apex.x + sinf(angle) * radius, apex.y, apex.z + cosf(angle) * radius };
		AddTriangle(apex, prev, curr, color);
		prev = curr;
	}
}

// a fan all around
void VfxRenderer::DrawGroundDisc(DirectX::XMFLOAT3 center, float radius, DirectX::XMFLOAT4 color, int segments)
{
	DrawGroundFan(center, 0.f, radius, DirectX::XM_PI, color, segments);
}

void VfxRenderer::Flush(Renderer& renderer, DirectX::FXMMATRIX view)
{
	if (vertices.empty())
	{
		return;
	}

	ID3D11DeviceContext* ctx = renderer.GetContext().Get();

	EnsureCapacity(renderer, vertices.size());
	{
		D3D11_MAPPED_SUBRESOURCE msr;
		ctx->Map(vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
		memcpy(msr.pData, vertices.data(), sizeof(DebugVertex) * vertices.size());
		ctx->Unmap(vertexBuffer.Get(), 0);
	}

	{
		// cam3D view + 3D proj, never the active ones (RenderSystem may have left 2D mode on)
		VfxTransforms t;
		t.model = DirectX::XMMatrixIdentity();
		t.view = DirectX::XMMatrixTranspose(view);
		t.proj = DirectX::XMMatrixTranspose(renderer.Get3DProj());

		D3D11_MAPPED_SUBRESOURCE msr;
		ctx->Map(transformCBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
		memcpy(msr.pData, &t, sizeof(t));
		ctx->Unmap(transformCBuffer.Get(), 0);
	}

	// remember whatever is bound (3D or 2D mode, any cull) to put it back afterwards.
	// the Get calls add a reference: ComPtr releases it at the end of this function
	Microsoft::WRL::ComPtr<ID3D11BlendState> prevBlend;
	float prevBlendFactor[4] = {};
	UINT prevSampleMask = 0;
	ctx->OMGetBlendState(&prevBlend, prevBlendFactor, &prevSampleMask);

	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> prevDepth;
	UINT prevStencilRef = 0;
	ctx->OMGetDepthStencilState(&prevDepth, &prevStencilRef);

	Microsoft::WRL::ComPtr<ID3D11RasterizerState> prevRaster;
	ctx->RSGetState(&prevRaster);

	// draw
	ctx->OMSetBlendState(blendAdditive.Get(), nullptr, 0xFFFFFFFF);
	ctx->OMSetDepthStencilState(depthTestNoWrite.Get(), 0);
	ctx->RSSetState(cullNone.Get());

	vertexShader->Bind(renderer);
	pixelShader->Bind(renderer);
	inputLayout->Bind(renderer);
	ctx->VSSetConstantBuffers(0, 1, transformCBuffer.GetAddressOf());
	ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	const UINT stride = sizeof(DebugVertex);
	const UINT offset = 0;
	ctx->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
	ctx->Draw(static_cast<UINT>(vertices.size()), 0);

	// restore
	ctx->OMSetBlendState(prevBlend.Get(), prevBlendFactor, prevSampleMask);
	ctx->OMSetDepthStencilState(prevDepth.Get(), prevStencilRef);
	ctx->RSSetState(prevRaster.Get());
}

void VfxRenderer::AddTriangle(const DirectX::XMFLOAT3& p0, const DirectX::XMFLOAT3& p1, const DirectX::XMFLOAT3& p2, const DirectX::XMFLOAT4& color)
{
	vertices.push_back({ p0, color });
	vertices.push_back({ p1, color });
	vertices.push_back({ p2, color });
}

void VfxRenderer::AddQuad(const DirectX::XMFLOAT3& p0, const DirectX::XMFLOAT3& p1, const DirectX::XMFLOAT3& p2, const DirectX::XMFLOAT3& p3, const DirectX::XMFLOAT4& color)
{
	AddTriangle(p0, p1, p2, color);
	AddTriangle(p0, p2, p3, color);
}

void VfxRenderer::EnsureCapacity(Renderer& renderer, size_t vertexCount)
{
	if (vertexCount <= vertexBufferCapacity && vertexBuffer)
	{
		return;
	}

	vertexBufferCapacity = vertexCount * 2 + 256;

	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bd.ByteWidth = static_cast<UINT>(sizeof(DebugVertex) * vertexBufferCapacity);
	bd.StructureByteStride = sizeof(DebugVertex);

	vertexBuffer.Reset();
	renderer.GetDevice()->CreateBuffer(&bd, nullptr, &vertexBuffer);
}
