#pragma once
#include "Component.h"
#include "DrawableBase.h"

class TransformComponent : public Component, public DrawableBase<TransformComponent>
{
public:
	TransformComponent(DirectX::XMFLOAT3 pos = {}, DirectX::XMFLOAT3 rot = {}, DirectX::XMFLOAT3 scale = {});

	DirectX::XMMATRIX GetMatrix() const;
public:
	DirectX::XMFLOAT3 position = {0, 0, 0};
	DirectX::XMFLOAT3 rotation = { 0, 0, 0 };
	DirectX::XMFLOAT3 scale = { 0, 0, 0 };
};

