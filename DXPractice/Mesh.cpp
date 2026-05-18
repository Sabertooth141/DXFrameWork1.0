#include "Mesh.h"

#include "InputLayout.h"
#include "MaterialCBuffer.h"
#include "PixelShader.h"
#include "Topology.h"
#include "TransformCBuffer.h"
#include "VertexBuffer.h"
#include "VertexShader.h"

Mesh::Mesh(Renderer& renderer, const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices)
{
    // static buffers
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
        AddStaticBind(std::make_unique<Topology>(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST));
    }

    // per instance
    AddBind(std::make_unique<VertexBuffer>(renderer, vertices));
    AddIndexBuffer(std::make_unique<IndexBuffer>(renderer, indices));

    AddBind(std::make_unique<TransformCBuffer>(renderer, *this));

    MaterialData mat = {};
    mat.color = { 1.0f, 1.0f, 1.0f };
    mat.specularIntensity = 1.0f;
    mat.specularPower = 32.0f;

    AddBind(std::make_unique<MaterialCBuffer>(renderer, mat));

}

void Mesh::Update(float deltaTime)
{
}

DirectX::XMMATRIX Mesh::GetTransformMatrix()
{
	return DirectX::XMMatrixTranslation(position.x, position.y, position.z);
}
