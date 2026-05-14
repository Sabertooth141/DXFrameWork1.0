#include "ConstantBuffer.h"

template <typename T>
ConstantBuffer<T>::ConstantBuffer(Renderer& renderer, const T& data)
{
	D3D11_BUFFER_DESC bd = {};
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bd.ByteWidth = sizeof(T);

	D3D11_SUBRESOURCE_DATA sd = {};
	sd.pSysMem = &data;

	GetDevice(renderer)->CreateBuffer(&bd, &sd, &pConstantBuffer);
}

template <typename T>
void ConstantBuffer<T>::Update(Renderer& renderer, const T& data)
{
	D3D11_MAPPED_SUBRESOURCE msr;
	GetContext(renderer)->Map(pConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	memcpy(msr.pData, &data, sizeof(T));
	GetContext(renderer)->Unmap(pConstantBuffer.Get(), 0);
}

template <typename T>
void PixelConstantBuffer<T>::Bind(Renderer& renderer)
{
	GetContext(renderer)->PSSetConstantBuffers(0, 1, pConstantBuffer.GetAddressOf());
}

template <typename T>
void VertexConstantBuffer<T>::Bind(Renderer& renderer)
{
	GetContext(renderer)->VSSetConstantBuffers(0, 1, pConstantBuffer.GetAddressOf());
}
