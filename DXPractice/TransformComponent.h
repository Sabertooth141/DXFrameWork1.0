#pragma once
#include "IComponent.h"
#include "DrawableBase.h"

class TransformComponent : public IComponent
{
public:
	TransformComponent(DirectX::XMFLOAT3 pos = {0, 0, 0},
	                   DirectX::XMFLOAT3 rot = {0, 0, 0},
	                   DirectX::XMFLOAT3 scale = {1, 1, 1});

	void SetPosition(DirectX::XMFLOAT3 inPosition);
	void SetRotation(DirectX::XMFLOAT3 inRotation);
	void SetScale(DirectX::XMFLOAT3 inScale);

	void Rotate(DirectX::XMFLOAT3 inRotate);
	void Translate(DirectX::XMFLOAT3 inTranslate);

	DirectX::XMMATRIX GetMatrix() const;

private:
	DirectX::XMFLOAT3 position = {0, 0, 0};
	DirectX::XMFLOAT3 rotation = {0, 0, 0};
	DirectX::XMFLOAT3 scale = {0, 0, 0};
};
