#include "TransformCBuffer.h"

#include "Drawable.h"

TransformCBuffer::TransformCBuffer(Renderer& renderer, Drawable& parent) : parent(parent), vCBuffer(renderer, Transforms{}, 0)
{
}

void TransformCBuffer::Bind(Renderer& renderer)
{
	Transforms transforms;

	transforms.model = DirectX::XMMatrixTranspose(parent.GetTransformMatrix());
	transforms.view = DirectX::XMMatrixTranspose(renderer.GetView());
	transforms.projection = DirectX::XMMatrixTranspose(renderer.GetProj());

	vCBuffer.Update(renderer, transforms);
	vCBuffer.Bind(renderer);
}
