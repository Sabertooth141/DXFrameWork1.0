#include "VertexBuffer.h"

template <typename V>
VertexBuffer::VertexBuffer(Renderer& renderer, const std::vector<V>& vertices)
{
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.ByteWidth = static_cast<UINT>(sizeof(V) * vertices.size());
	bd.StructureByteStride = sizeof(V);

	D3D11_SUBRESOURCE_DATA sd = {};
	sd.pSysMem = vertices.data();
	
	GetDevice(renderer)->CreateBuffer(&bd, &sd, &pVertexBuffer);
	stride = sizeof(V);
}

void VertexBuffer::Bind(Renderer& renderer)
{
	const UINT offset = 0;
	GetContext(renderer)->IASetVertexBuffers(0, 1, pVertexBuffer.GetAddressOf(), &stride, &offset);
}
