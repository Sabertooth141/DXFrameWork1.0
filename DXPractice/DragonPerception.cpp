#include "DragonPerception.h"

#include <cmath>

#include "DebugRenderer.h"

void Perception::Update(const DirectX::XMFLOAT3& selfPos, float selfYaw, const DirectX::XMFLOAT3& targetPos)
{
	hasTarget = true;

	const float dx = targetPos.x - selfPos.x;
	const float dz = targetPos.z - selfPos.z;
	distance = sqrt(dx * dx + dz * dz);

	if (distance > 1e-3f)
	{
		const float toTargetYaw = atan2f(dx, dz);
		angleDeg = DirectX::XMConvertToDegrees(remainderf(toTargetYaw - selfYaw, DirectX::XM_2PI));
	}

	band = ClassifyBand(distance);
	sector = ClassifySector(angleDeg);

	this->targetPos = targetPos;
}

void Perception::ClearTarget()
{
	hasTarget = false;
}

#ifndef NDEBUG
void Perception::DrawDebug(DebugRenderer& debugRenderer, const DirectX::XMFLOAT3& selfPos, float selfYaw) const
{
	const DirectX::XMFLOAT3 center{selfPos.x, selfPos.y + 0.05f, selfPos.z};

	const DirectX::XMFLOAT4 gray{0.6f, 0.6f, 0.6f, 1.f};
	const DirectX::XMFLOAT4 yellow{1.f, 0.85f, 0.2f, 1.f};
	const DirectX::XMFLOAT4 green{0.2f, 1.f, 0.3f, 1.f};
	const DirectX::XMFLOAT4 red{1.f, 0.25f, 0.2f, 1.f};
	const DirectX::XMFLOAT4 blue{0.3f, 0.6f, 1.f, 1.f};

	// point at an angle relative to the dragon's forward (+ = right), same convention as angleDeg
	const auto pointAt = [&](float relDeg, float radius)
	{
		const float yaw = selfYaw + DirectX::XMConvertToRadians(relDeg);
		return DirectX::XMFLOAT3{center.x + sinf(yaw) * radius, center.y, center.z + cosf(yaw) * radius};
	};

	// band rings: the outer edge of the current band in yellow
	debugRenderer.DrawCircleXZ(center, closeMax, band == DistanceBand::CLOSE ? yellow : gray);
	debugRenderer.DrawCircleXZ(center, midMax, band == DistanceBand::MID ? yellow : gray);

	// sector edges out to the mid ring, forward in blue
	for (const float edgeDeg : {frontHalfDeg, -frontHalfDeg, backStartDeg, -backStartDeg})
	{
		debugRenderer.DrawLine3D(center, pointAt(edgeDeg, midMax), gray);
	}
	debugRenderer.DrawLine3D(center, pointAt(0.f, closeMax), blue);

	// line to the target, colored by sector
	if (hasTarget)
	{
		const DirectX::XMFLOAT4 sectorColor =
			sector == DirSector::FRONT ? green : sector == DirSector::BACK ? red : yellow;
		debugRenderer.DrawLine3D(center, {targetPos.x, center.y, targetPos.z}, sectorColor);
	}
}
#endif

DistanceBand Perception::ClassifyBand(float dist) const
{
	const float closeEdge = closeMax + (band == DistanceBand::CLOSE ? distanceHysteresis : -distanceHysteresis);
	const float midEdge = midMax + (band == DistanceBand::DISTANT ? -distanceHysteresis : distanceHysteresis);

	if (dist < closeEdge)
	{
		return DistanceBand::CLOSE;
	}
	if (dist < midEdge)
	{
		return DistanceBand::MID;
	}

	return DistanceBand::DISTANT;
}

DirSector Perception::ClassifySector(float angle) const
{
	const float absAngle = std::fabs(angle);
	const float frontEdge = frontHalfDeg + (sector == DirSector::FRONT ? angleHysteresis : -angleHysteresis);
	const float backEdge = backStartDeg + (sector == DirSector::BACK ? -angleHysteresis : angleHysteresis);

	if (absAngle < frontEdge)
	{
		return DirSector::FRONT;
	}
	if (absAngle > backEdge)
	{
		return DirSector::BACK;
	}

	return angle > 0.f ? DirSector::RIGHT : DirSector::LEFT;
}

const char* ToString(DistanceBand band)
{
	switch (band)
	{
	case DistanceBand::CLOSE:
		return "CLOSE";
	case DistanceBand::MID:
		return "MID";
	case DistanceBand::DISTANT:
		return "DISTANT";
	}
	return "?";
}

const char* ToString(DirSector sector)
{
	switch (sector)
	{
	case DirSector::FRONT:
		return "FRONT";
	case DirSector::LEFT:
		return "LEFT";
	case DirSector::RIGHT:
		return "RIGHT";
	case DirSector::BACK:
		return "BACK";
	}
	return "?";
}
