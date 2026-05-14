#pragma once
#include "Bindable.h"

template<typename T>
class ConstantBuffer : public Bindable
{
public:
	ConstantBuffer(Renderer& renderer, const T& data);
	void Update(Renderer& renderer, const T& data);

protected:
	Microsoft::WRL::ComPtr<ID3D11Buffer> pConstantBuffer;
};

template<typename T>
class PixelConstantBuffer : public ConstantBuffer<T>
{
	using ConstantBuffer<T>::pConstantBuffer;
	using ConstantBuffer<T>::GetContext;

public:
	using ConstantBuffer<T>::ConstantBuffer;

	void Bind(Renderer& renderer) override;
};

template<typename T>
class VertexConstantBuffer : public ConstantBuffer<T>
{
	using ConstantBuffer<T>::pConstantBuffer;
	using ConstantBuffer<T>::GetContext;

public:
	using ConstantBuffer<T>::ConstantBuffer;

	void Bind(Renderer& renderer) override;
};
