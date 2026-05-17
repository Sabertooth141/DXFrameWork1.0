#pragma once
#include "DrawableBase.h"

class Box : public DrawableBase<Box>
{
public:
	Box(Renderer& renderer);
	DirectX::XMMATRIX GetTransformMatrix() override;
	void Update(float deltaTime) override;

private:
	float angle = 0;
};

