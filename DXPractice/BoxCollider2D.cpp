#include "BoxCollider2D.h"
#include "TransformComponent.h"

BoxCollider2D::BoxCollider2D(const DirectX::XMFLOAT2 inHalfExtents, const DirectX::XMFLOAT2 inOffset,
                             const bool inIsTrigger,
                             TransformComponent& inTransformComp) :
	transformComp(&inTransformComp),
	halfExtents(inHalfExtents),
	offset(inOffset),
	isTrigger(inIsTrigger)
{
}

AABB BoxCollider2D::GetWorldAABB() const
{
	const DirectX::XMFLOAT3 position = transformComp->GetPosition();
	const DirectX::XMFLOAT3 scale = transformComp->GetScale();

	const DirectX::XMFLOAT2 worldCenter =
	{
		position.x + offset.x * scale.x,
		position.y + offset.y * scale.y
	};

	const DirectX::XMFLOAT2 worldHalfExtents =
	{
		halfExtents.x * scale.x,
		halfExtents.y * scale.y
	};

	return AABB{
		.min = {worldCenter.x - worldHalfExtents.x, worldCenter.y - worldHalfExtents.y},
		.max = {worldCenter.x + worldHalfExtents.x, worldCenter.y + worldHalfExtents.y}
	};
}
