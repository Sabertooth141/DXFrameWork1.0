#pragma once
#include "Bindable.h"

class Texture : public Bindable
{
public:
	Texture(ID3D11ShaderResourceView* inSrv, UINT inSlot = 0) : srv(inSrv), slot(inSlot)
	{
	}

	void Bind(Renderer& renderer) override
	{
		GetContext(renderer)->PSSetShaderResources(slot, 1, &srv);
	}

private:
	ID3D11ShaderResourceView* srv;
	UINT slot;
};
