#include "TransformComponent.h"

TransformComponent::TransformComponent(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 rot, DirectX::XMFLOAT3 scale) :
	position(pos),
	rotation(rot),
	scale(scale)
{
}

DirectX::XMMATRIX TransformComponent::GetMatrix() const
{
	return DirectX::XMMatrixScaling(scale.x, scale.y, scale.z)
		* DirectX::XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z)
		* DirectX::XMMatrixTranslation(position.x, position.y, position.z);
}
