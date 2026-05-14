#include "Renderer.h"

#pragma comment(lib, "d3d11.lib")

Renderer::Renderer(HWND hWnd, int width, int height)
{
	// device & swap chain
	DXGI_SWAP_CHAIN_DESC sd = {};
	sd.BufferDesc.Width = width;
	sd.BufferDesc.Height = height;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 0;
	sd.BufferDesc.RefreshRate.Denominator = 0;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = 1;
	sd.OutputWindow = hWnd;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	sd.Flags = 0;

	UINT flags = 0;
#ifdef _DEBUG
	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		flags,
		nullptr,
		0,
		D3D11_SDK_VERSION,
		&sd,
		&pSwapChain,
		&pDevice,
		nullptr,
		&pContext
	);

	// render target view
	Microsoft::WRL::ComPtr<ID3D11Resource> pBackBuffer;
	pSwapChain->GetBuffer(0, __uuidof(ID3D11Resource), &pBackBuffer);
	pDevice->CreateRenderTargetView(pBackBuffer.Get(), nullptr, &pRTV);

	// depth stencil buffer + view
	D3D11_DEPTH_STENCIL_DESC dsDesc = {};
	dsDesc.DepthEnable = TRUE;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS;

	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> pDSState;
	pDevice->CreateDepthStencilState(&dsDesc, &pDSState);
	pContext->OMSetDepthStencilState(pDSState.Get(), 1);

	Microsoft::WRL::ComPtr<ID3D11Texture2D> pDepthStencil;
	D3D11_TEXTURE2D_DESC dtDesc = {};
	dtDesc.Width = width;
	dtDesc.Height = height;
	dtDesc.MipLevels = 1;
	dtDesc.ArraySize = 1;
	dtDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dtDesc.SampleDesc.Count = 1;
	dtDesc.SampleDesc.Quality = 0;
	dtDesc.Usage = D3D11_USAGE_DEFAULT;
	dtDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	pDevice->CreateTexture2D(&dtDesc, nullptr, &pDepthStencil);

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;
	pDevice->CreateDepthStencilView(pDepthStencil.Get(), &dsvDesc, &pDSV);

	// bind render target + depth stencil
	pContext->OMSetRenderTargets(1, pRTV.GetAddressOf(), pDSV.Get());

	// viewport
	D3D11_VIEWPORT vp = {};
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	vp.Width = static_cast<float>(width);
	vp.Height = static_cast<float>(height);
	vp.MinDepth = 0;
	vp.MaxDepth = 1;
	pContext->RSSetViewports(1, &vp);
}

void Renderer::BeginFrame(float r, float g, float b)
{
	const float color[] = {r, g, b, 1};
	pContext->ClearRenderTargetView(pRTV.Get(), color);
	pContext->ClearDepthStencilView(pDSV.Get(), D3D11_CLEAR_DEPTH, 1, 0);
}

void Renderer::EndFrame()
{
	pSwapChain->Present(1, 0);
}

void Renderer::DrawIndexed(UINT count)
{
	pContext->DrawIndexed(count, 0, 0);
}
