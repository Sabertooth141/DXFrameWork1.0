#include "MaterialComponent.h"

#include "InputLayout.h"
#include "MaterialCBuffer.h"
#include "PixelShader.h"
#include "VertexShader.h"

MaterialComponent::MaterialComponent(Renderer& renderer, MaterialData& matData)
{
	if (!IsStaticInit())
	{
		auto vShader = std::make_unique<VertexShader>(renderer, L"VertexShader.cso");
		auto vsByteCode = vShader->GetByteCode();

		AddStaticBind(std::move(vShader));
		AddStaticBind(std::make_unique<PixelShader>(renderer, L"PixelShader.cso"));

		const std::vector<D3D11_INPUT_ELEMENT_DESC> layout =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0}
		};

		AddStaticBind(std::make_unique<InputLayout>(renderer, layout, vsByteCode));
	}

	AddBind(std::make_unique<MaterialCBuffer>(renderer, matData));
}

void MaterialComponent::Draw(Renderer& renderer)
{
}
