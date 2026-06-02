#pragma once
#include <DirectXMath.h>
#include <string>

#include "ConstantBuffer.h"
#include "Drawable.h"
#include "IComponent.h"
#include "PixelShader.h"
#include "Sampler.h"
#include "Texture.h"
#include "UVTransformData.h"

class SpriteRendererComponent : public IComponent
{
public:
	SpriteRendererComponent(Renderer& renderer, ID3D11ShaderResourceView* srv);

	void Update(float deltaTime) override;
	void Bind(Renderer& renderer);
	void UpdateUV(const UVTransformData& data);

private:
	Renderer& renderer;
	PixelConstantBuffer<UVTransformData> uvCBuffer;
	Texture texture;
	Sampler sampler;
};

