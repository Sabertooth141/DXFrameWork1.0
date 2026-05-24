#pragma once
#include "DrawableBase.h"

struct Vertex
{
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT2 texcoord;
};

class Mesh : public DrawableBase<Mesh>
{
public:
	Mesh(Renderer& renderer,
		const std::vector<Vertex>& vertices,
		const std::vector<unsigned int>& indices);

private:
	DirectX::XMFLOAT3 position = {0, 0, 0};
};
