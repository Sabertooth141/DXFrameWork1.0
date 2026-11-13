#pragma once
#include <DirectXMath.h>

struct Capsule3D
{
	DirectX::XMFLOAT3 centerA;
	DirectX::XMFLOAT3 centerB;
	float radius = 0.f;
};

// closest points between axes
struct SegmentClosest3D
{
	DirectX::XMFLOAT3 pointAxisA;
	DirectX::XMFLOAT3 pointAxisB;
	float alongAxisA;
	float alongAxisB;
	float distSq = 0.f;
};

struct CapsuleContact3D
{
	bool isOverlap = false;

	DirectX::XMFLOAT3 closestOnA{};
	DirectX::XMFLOAT3 closestOnB{};

	DirectX::XMFLOAT3 normal{};
	float depth = 0.f;
	DirectX::XMFLOAT3 contactPoint{};
};

SegmentClosest3D ClosestPointsPerSegment(DirectX::FXMVECTOR startPtA, DirectX::FXMVECTOR endPtA,
                                         DirectX::FXMVECTOR startPtB, DirectX::GXMVECTOR endPtB);

CapsuleContact3D TestCapsuleVsCapsule(const Capsule3D& capA, const Capsule3D& capB);