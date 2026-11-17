#include "DragonPerception.h"

#include <cmath>

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
}

void Perception::ClearTarget()
{
	hasTarget = false;
}

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
