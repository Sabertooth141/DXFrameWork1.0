#pragma once
#include <DirectXMath.h>

struct MaterialData
{
	DirectX::XMFLOAT3 color = {1.f, 1.f, 1.f};
	float specularIntensity;
	float specularPower;

	DirectX::XMFLOAT3 padding;
};
