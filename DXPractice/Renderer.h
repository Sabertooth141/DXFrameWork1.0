#pragma once
#include "Win.h"

#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl.h>

class Bindable;

class Renderer
{
	friend class Bindable;

public:
	Renderer(HWND hWnd, int width, int height);
	Renderer(const Renderer&) = default;
	Renderer& operator =(const Renderer&) = default;
	~Renderer() = default;

	void BeginFrame(float r, float g, float b);
	void EndFrame();
	void DrawIndexed(UINT count);

	DirectX::XMMATRIX GetView()
	{
		return viewMatrix;
	}

	DirectX::XMMATRIX GetProj()
	{
		return projMatrix;
	}

private:
	Microsoft::WRL::ComPtr<ID3D11Device> pDevice;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> pContext;
	Microsoft::WRL::ComPtr<IDXGISwapChain> pSwapChain;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> pRTV;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> pDSV;

	DirectX::XMMATRIX viewMatrix;
	DirectX::XMMATRIX projMatrix;
};
