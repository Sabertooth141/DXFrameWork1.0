#include "Collision3D.h"

#include <algorithm>

namespace
{
	// segment shorter than .01mm -> treat as points
	constexpr float degenerateLenSq = 1e-10f;

	// angle below .06deg
	constexpr float parallelSinSq = 1e-6f;

	// too close
	constexpr float coincidentDistSq = 1e-12f;

	float ClampVal(float value)
	{
		return std::clamp(value, 0.f, 1.f);
	}

	float Dot(DirectX::FXMVECTOR vectorA, DirectX::FXMVECTOR vectorB)
	{
		return DirectX::XMVectorGetX(DirectX::XMVector3Dot(vectorA, vectorB));
	}

	// unit vector perpendicular to (dir != 0)
	DirectX::XMVECTOR FindPerpendicular(DirectX::FXMVECTOR dir)
	{
		using namespace DirectX;
		const XMVECTOR normal = XMVector3Normalize(dir);
		XMVECTOR helper;
		if (fabsf(XMVectorGetY(normal)) < 0.99f)
		{
			helper = XMVectorSet(0.f, 1.f, 0.f, 0.f);
		}
		else
		{
			helper = XMVectorSet(1.f, 0.f, 0.f, 0.f);
		}

		return XMVector3Normalize(XMVector3Cross(helper, dir));
	}

	// capsule separation dir when axes touch
	DirectX::XMVECTOR FallbackNormal(DirectX::FXMVECTOR axisA, DirectX::FXMVECTOR axisB)
	{
		using namespace DirectX;
		const float lenSqA = Dot(axisA, axisA);
		const float lenSqB = Dot(axisB, axisB);

		// crossing & perpendicular
		if (lenSqA > degenerateLenSq && lenSqB > degenerateLenSq)
		{
			const XMVECTOR cross = XMVector3Cross(axisA, axisB);
			if (Dot(cross, cross) > parallelSinSq * lenSqA * lenSqB)
			{
				return XMVector3Normalize(cross);
			}
		}

		// parallel / one is a sphere
		if (lenSqA > degenerateLenSq)
		{
			return FindPerpendicular(axisA);
		}
		if (lenSqB > degenerateLenSq)
		{
			return FindPerpendicular(axisB);
		}

		// two spheres with same center
		return XMVectorSet(0.f, 1.f, 0.f, 0.f);
	}
}

SegmentClosest3D ClosestPointsPerSegment(DirectX::FXMVECTOR startPtA, DirectX::FXMVECTOR endPtA, DirectX::FXMVECTOR startPtB, DirectX::GXMVECTOR endPtB)
{
	using namespace DirectX;

	const XMVECTOR dirA = endPtA - startPtA;
	const XMVECTOR dirB = endPtB - startPtB;
	const XMVECTOR startOffset = startPtA - startPtB;
	const float lenSqA = Dot(dirA, dirA);
	const float lenSqB = Dot(dirB, dirB);
	const float f = Dot(dirB, startOffset);

	float s = 0.f;
	float t = 0.f;

	// pt vs pt
	if (lenSqA <= degenerateLenSq && lenSqB <= degenerateLenSq)
	{
	}
	// pt vs segment
	else if (lenSqA <= degenerateLenSq)
	{
		// project A onto segB
		t = ClampVal(f / lenSqB);
	}
	else
	{
		const float c = Dot(dirA, startOffset);
		// seg vs pt
		if (lenSqB <= degenerateLenSq)
		{
			// proj B onto segA
			s = ClampVal(-c / lenSqA);
		}
		else
		{
			const float b = Dot(dirA, dirB);
			const float denom = lenSqA * lenSqB - b * b;

			// not parallel = closest pt ; parallel = s = 0
			if (denom > parallelSinSq * lenSqA * lenSqB)
			{
				s = ClampVal((b * f - c * lenSqB) / denom);
			}

			t = (b * s + f) / lenSqB;

			// not on segment
			if (t < 0.f)
			{
				t = 0.f;
				s = ClampVal(-c / lenSqA);
			}
			else if (t > 1.f)
			{
				t = 1.f;
				s = ClampVal((b - c) / lenSqA);
			}
		}
	}

	const XMVECTOR closestOnA = startPtA + dirA * s;
	const XMVECTOR closestOnB = startPtB + dirB * t;
	const XMVECTOR diff = closestOnA - closestOnB;

	SegmentClosest3D result;
	XMStoreFloat3(&result.pointAxisA, closestOnA);
	XMStoreFloat3(&result.pointAxisB, closestOnB);
	result.alongAxisA = s;
	result.alongAxisB = t;
	result.distSq = Dot(diff, diff);
	return result;
}

CapsuleContact3D TestCapsuleVsCapsule(const Capsule3D& capA, const Capsule3D& capB)
{
	using namespace DirectX;

	const XMVECTOR startA = XMLoadFloat3(&capA.centerA);
	const XMVECTOR endA = XMLoadFloat3(&capA.centerB);
	const XMVECTOR startB = XMLoadFloat3(&capB.centerA);
	const XMVECTOR endB = XMLoadFloat3(&capB.centerB);

	const SegmentClosest3D closest = ClosestPointsPerSegment(startA, endA, startB, endB);

	CapsuleContact3D result;
	result.closestOnA = closest.pointAxisA;
	result.closestOnB = closest.pointAxisB;

	// overlap when segment dist < combined radii
	const float radiusSum = capA.radius + capB.radius;
	// no contact
	if (closest.distSq >= radiusSum * radiusSum)
	{
		return result;
	}

	const XMVECTOR onA = XMLoadFloat3(&closest.pointAxisA);
	const XMVECTOR onB = XMLoadFloat3(&closest.pointAxisB);

	XMVECTOR normal;
	float dist;

	if (closest.distSq > coincidentDistSq)
	{
		dist = sqrtf(closest.distSq);
		normal = (onA - onB) / dist;
	}
	else
	{
		dist = 0.f;
		normal = FallbackNormal(endA - startA, endB - startB);
	}

	// surface pts
	const XMVECTOR surfaceA = onA - normal * capA.radius;
	const XMVECTOR surfaceB = onB + normal * capB.radius;

	result.isOverlap = true;
	result.depth = radiusSum - dist;
	XMStoreFloat3(&result.normal, normal);
	XMStoreFloat3(&result.contactPoint, (surfaceA + surfaceB) * 0.5f);
	return result;
}