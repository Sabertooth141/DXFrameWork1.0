#pragma once
#include <DirectXMath.h>

#include "Bindable.h"
#include "ConstantBuffer.h"

class Drawable;

struct Transforms
{
	DirectX::XMMATRIX model;
	DirectX::XMMATRIX view;
	DirectX::XMMATRIX projection;
};

class TransformCBuffer : public Bindable
{
public:
	TransformCBuffer(Renderer& renderer, Drawable& parent);

	void Bind(Renderer& renderer) override;

private:
	Drawable& parent;
	VertexConstantBuffer<Transforms> vCBuffer;
};

