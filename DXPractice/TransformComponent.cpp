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

void TransformComponent::SetPosition(DirectX::XMFLOAT3 inPosition)
{

	position = inPosition;
}

void TransformComponent::SetRotation(DirectX::XMFLOAT3 inRotation)
{
	rotation = inRotation;
}

void TransformComponent::SetScale(DirectX::XMFLOAT3 inScale)
{
	scale = inScale;
}

void TransformComponent::Rotate(DirectX::XMFLOAT3 inRotate)
{
	rotation = DirectX::XMFLOAT3(rotation.x + inRotate.x, rotation.y + inRotate.y, rotation.z + inRotate.z);
}

void TransformComponent::Translate(DirectX::XMFLOAT3 inTranslate)
{
	position = DirectX::XMFLOAT3(position.x + inTranslate.x, position.y + inTranslate.y, position.z + inTranslate.z);
}
