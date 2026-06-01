#include "SpriteRendererComponent.h"

struct SpriteVertex
{
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT2 uv;
};

SpriteRendererComponent::SpriteRendererComponent(Renderer& renderer, ID3D11ShaderResourceView* srv)
    : renderer(renderer)
    , uvCBuffer(renderer, UVTransformData{ {0.f, 0.f}, {1.f, 1.f} }, 1) // b1
    , texture(srv, 0)    // t0
    , sampler(renderer, 0) // s0

{
}

void SpriteRendererComponent::Bind(Renderer& renderer)
{
    uvCBuffer.Bind(renderer);
    texture.Bind(renderer);
    sampler.Bind(renderer);
}

void SpriteRendererComponent::UpdateUV(const UVTransformData& data)
{
    uvCBuffer.Update(renderer, data);
}
