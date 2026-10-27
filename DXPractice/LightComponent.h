#pragma once
#include <DirectXMath.h>

#include "IComponent.h"
struct LightData;

class LightComponent : public IComponent
{
public:
	LightData ToLightData() const;

	void OnAttach(GameContext& context) override;

public:
	bool isEnabled = true;
	DirectX::XMFLOAT3 color = {1.f, 1.f, 1.f};
	float intensity = 1.f;
	DirectX::XMFLOAT3 ambient{ 0.15f, 0.15f, 0.15f };

	float attConst = 1.f;
	float attLin = 0.045f;
	float attQuad = 0.0075f;
};

