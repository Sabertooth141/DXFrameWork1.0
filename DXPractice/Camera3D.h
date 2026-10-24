#pragma once
#include <algorithm>
#include <DirectXMath.h>

class Camera3D
{
public:
	DirectX::XMMATRIX GetView() const
	{
		return DirectX::XMMatrixLookToLH(DirectX::XMLoadFloat3(&pos), GetCamForward(), {0.f, 1.f, 0.f});
	}

	DirectX::XMVECTOR GetCamForward() const
	{
		return DirectX::XMVectorSet(cosf(pitch) * sinf(yaw), sinf(pitch), cosf(pitch) * cosf(yaw), 0.f);
	}

	DirectX::XMVECTOR GetCamRight() const
	{
		return DirectX::XMVectorSet(cosf(yaw), 0.f, -sinf(yaw), 0.f);
	}

	void Rotate(float dYaw, float dPitch)
	{
		yaw += dYaw;
		pitch = std::clamp(pitch + dPitch, -1.5f, 1.5f); // plus minus 86
	}

public:
	DirectX::XMFLOAT3 pos = {0.f, 2.f, -8.f};
	float yaw = 0.f;
	float pitch = 0.f;
};
