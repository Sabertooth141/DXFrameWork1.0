#include "InputLayout.h"

InputLayout::InputLayout(Renderer& renderer, const std::vector<D3D11_INPUT_ELEMENT_DESC>& layout,
	ID3DBlob* pVertexShaderByteCode)
{
	const HRESULT hr = GetDevice(renderer)->CreateInputLayout(
		layout.data(),
		static_cast<UINT>(layout.size()),
		pVertexShaderByteCode->GetBufferPointer(),
		pVertexShaderByteCode->GetBufferSize(),
		&pInputLayout
	);
	assert(SUCCEEDED(hr) && "CreateInputLayout failed - check VS input semantics against the layout");
}

void InputLayout::Bind(Renderer& renderer)
{
	GetContext(renderer)->IASetInputLayout(pInputLayout.Get());
}
