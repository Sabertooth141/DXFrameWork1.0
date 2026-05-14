#pragma once
#include <vector>

#include "Bindable.h"

class VertexBuffer : public Bindable
{
public:
	template<typename V>
	VertexBuffer(Renderer& renderer, const std::vector<V>& vertices);
	void Bind(Renderer& renderer) override;

private:
	UINT stride;
	Microsoft::WRL::ComPtr<ID3D11Buffer> pVertexBuffer;
};

