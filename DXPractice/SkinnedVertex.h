#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>

// for rigged models | stores bound bones and respective weights
struct SkinnedVertex
{
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT2 texcoord;
	uint8_t boneIndices[4];
	float boneWeights[4];

	static const std::vector<D3D11_INPUT_ELEMENT_DESC>& GetInputLayout()
	{
		static const std::vector<D3D11_INPUT_ELEMENT_DESC> layout =
		{
			{
				"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(SkinnedVertex, pos),
				D3D11_INPUT_PER_VERTEX_DATA, 0
			},
			{
				"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(SkinnedVertex, normal),
				D3D11_INPUT_PER_VERTEX_DATA, 0
			},
			{
				"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(SkinnedVertex, texcoord),
				D3D11_INPUT_PER_VERTEX_DATA, 0
			},
			{
				"BLENDINDICES", 0, DXGI_FORMAT_R8G8B8A8_UINT, 0, offsetof(SkinnedVertex, boneIndices),
				D3D11_INPUT_PER_VERTEX_DATA, 0
			},
			{
				"BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(SkinnedVertex, boneWeights),
				D3D11_INPUT_PER_VERTEX_DATA, 0
			}
		};

		return layout;
	}
};

static_assert(sizeof(SkinnedVertex) == 52, "SkinnedVertex must be tightly packed for the input layout");