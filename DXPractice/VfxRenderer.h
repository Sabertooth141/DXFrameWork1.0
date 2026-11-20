#pragma once
#include <DirectXMath.h>
#include <memory>
#include <vector>
#include <wrl/client.h>

#include "ConstantBuffer.h"
class InputLayout;
class PixelShader;
class VertexShader;
struct DebugVertex;
class Renderer;

class VfxRenderer
{
public:
	explicit VfxRenderer(Renderer& renderer);
	~VfxRenderer();

	VfxRenderer(const VfxRenderer&) = delete;
	VfxRenderer& operator=(const VfxRenderer&) = delete;

	void Begin();
	void DrawCapsule(DirectX::XMFLOAT3 centerA, DirectX::XMFLOAT3 centerB, float radius, DirectX::XMFLOAT4 color);

	void DrawGroundQuad(DirectX::XMFLOAT3 center, float yaw, float halfWidth, float halfLength, DirectX::XMFLOAT4 color);
	// starts at start and runs length along yaw
	void DrawGroundLine(DirectX::XMFLOAT3 start, float yaw, float length, float width, DirectX::XMFLOAT4 color);
	// pie slice with its tip at apex, centered on yaw, halfAngleRad to each side
	void DrawGroundFan(DirectX::XMFLOAT3 apex, float yaw, float radius, float halfAngleRad, DirectX::XMFLOAT4 color, int segments = 24);
	void DrawGroundDisc(DirectX::XMFLOAT3 center, float radius, DirectX::XMFLOAT4 color, int segments = 32);

	void Flush(Renderer& renderer, DirectX::FXMMATRIX view);

private:
	void AddTriangle(const DirectX::XMFLOAT3& p0, const DirectX::XMFLOAT3& p1, const DirectX::XMFLOAT3& p2, const DirectX::XMFLOAT4& color);

	void AddQuad(const DirectX::XMFLOAT3& p0, const DirectX::XMFLOAT3& p1, const DirectX::XMFLOAT3& p2, const DirectX::XMFLOAT3& p3, const DirectX::XMFLOAT4& color);
	void EnsureCapacity(Renderer& renderer, size_t vertexCount);

private:
	std::vector<DebugVertex> vertices;

	std::unique_ptr<VertexShader> vertexShader;
	std::unique_ptr<PixelShader> pixelShader;
	std::unique_ptr<InputLayout> inputLayout;

	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> transformCBuffer;
	size_t vertexBufferCapacity = 0;

	// own states: Renderer stays untouched
	Microsoft::WRL::ComPtr<ID3D11BlendState> blendAdditive;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthTestNoWrite;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> cullNone;

	static constexpr int capsuleSegments = 12;
	static constexpr int capsuleCapRings = 4;
};
