#define STB_IMAGE_IMPLEMENTATION

#include "TextureCache.h"
#include "stb_image.h"

std::unordered_map<std::wstring, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> TextureCache::cache;

ID3D11ShaderResourceView* TextureCache::Load(Renderer& renderer, const std::wstring& path)
{
	auto it = cache.find(path);
	if (it != cache.end())
	{
		return it->second.Get();
	}

	std::string narrowPath(path.begin(), path.end());

	int width, height, channels;
	void* pixels = nullptr;
	DXGI_FORMAT fmt;
	UINT bytesPerPixel;


	if (stbi_is_hdr(narrowPath.c_str())) {
		// --- HDR / float ---
		pixels = stbi_loadf(narrowPath.c_str(), &width, &height, &channels, 4);
		fmt = DXGI_FORMAT_R32G32B32A32_FLOAT;
		bytesPerPixel = 16;
	}
	else if (stbi_is_16_bit(narrowPath.c_str())) {
		// --- 16-bit ---
		pixels = stbi_load_16(narrowPath.c_str(), &width, &height, &channels, 4);
		fmt = DXGI_FORMAT_R16G16B16A16_UNORM;
		bytesPerPixel = 8;
	}
	else {
		// --- 8-bit (default) ---
		pixels = stbi_load(narrowPath.c_str(), &width, &height, &channels, 4);
		fmt = DXGI_FORMAT_R8G8B8A8_UNORM;
		bytesPerPixel = 4;
	}

	assert(pixels && "TextureCache: stbi_load failed");

	D3D11_TEXTURE2D_DESC td = {};
	td.Width = static_cast<UINT>(width);
	td.Height = static_cast<UINT>(height);
	td.MipLevels = 1;
	td.ArraySize = 1;
	td.Format = fmt;
	td.SampleDesc.Count = 1;
	td.SampleDesc.Quality = 0;
	td.Usage = D3D11_USAGE_DEFAULT;
	td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA sd = {};
	sd.pSysMem = pixels;
	sd.SysMemPitch = width * 4;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> pTexture;
	HRESULT hr = GetDevice(renderer)->CreateTexture2D(&td, &sd, &pTexture);
	assert(SUCCEEDED(hr) && "TextureCache: CreateTexture2D failed");

	// create SRV
	D3D11_SHADER_RESOURCE_VIEW_DESC srvd = {};
	srvd.Format = fmt;
	srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvd.Texture2D.MostDetailedMip = 0;
	srvd.Texture2D.MipLevels = 1;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
	hr = GetDevice(renderer)->CreateShaderResourceView(pTexture.Get(), &srvd, &srv);
	assert(SUCCEEDED(hr) && "TextureCache: CreateShaderResourceView failed");

	cache[path] = std::move(srv);
	return cache[path].Get();
}

void TextureCache::Clear()
{
	cache.clear();
}

void TextureCache::Bind(Renderer& renderer)
{
	return;
}
