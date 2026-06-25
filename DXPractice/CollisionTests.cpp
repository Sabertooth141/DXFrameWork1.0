#include "CollisionTests.h"

std::optional<CollisionManifold> TestBoxVsBox(const OBB& a, const OBB& b)
{
	const std::array<DirectX::XMFLOAT2, 2> axesA = a.GetAxes();
	const std::array<DirectX::XMFLOAT2, 2> axesB = b.GetAxes();
	const std::array<DirectX::XMFLOAT2, 4> axes =
	{
		axesA[0], axesA[1],
		axesB[0], axesB[1]
	};

	const auto cornersA = a.GetCorners();
	const auto cornersB = b.GetCorners();

	float minOverlap = FLT_MAX;
	DirectX::XMFLOAT2 smallestAxis = {0, 0};

	// projections
	for (const auto& axis : axes)
	{
		float minA = FLT_MAX;
		float maxA = -FLT_MAX;
		for (const auto& corner : cornersA)
		{
			// take dot product of corners projected onto normal
			const float proj = corner.x * axis.x + corner.x * axis.y;
			minA = std::min(minA, proj);
			maxA = std::max(maxA, proj);
		}

		float minB = FLT_MAX;
		float maxB = -FLT_MAX;
		for (const auto& corner : cornersB)
		{
			// take dot product of corners projected onto normal
			const float proj = corner.x * axis.x + corner.x * axis.y;
			minB = std::min(minB, proj);
			maxB = std::max(maxB, proj);
		}

		// test overlap, if no overlap on one axis -> no collision
		const float overlap = std::min(maxA, maxB) - std::max(minA, minB);
		if (overlap <= 0)
		{
			return std::nullopt;
		}

		// find the tightest overlap for physics 
		if (overlap < minOverlap)
		{
			minOverlap = overlap;
			smallestAxis = axis;
		}
	}

	return CollisionManifold{.normal = smallestAxis, .penetration = minOverlap};
}

void RegisterCollisionTests()
{
	CollisionDispatch::GetInstance().Register(ColliderType::Box, ColliderType::Box,
	                                          [](const Collider2D& a, const Collider2D& b) -> std::optional<CollisionManifold>
	                                          {
		                                          const auto& boxA = dynamic_cast<const BoxCollider2D&>(a);
		                                          const auto& boxB = dynamic_cast<const BoxCollider2D&>(b);
												  return TestBoxVsBox(boxA.GetWorldOBB(), boxB.GetWorldOBB());
	                                          });
}
