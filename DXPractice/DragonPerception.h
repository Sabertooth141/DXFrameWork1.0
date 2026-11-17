#pragma once
#include <cstdint>
#include <DirectXMath.h>

enum class DistanceBand : uint8_t
{
	CLOSE,
	MID,
	DISTANT
};

enum class DirSector : uint8_t
{
	FRONT,
	LEFT,
	RIGHT,
	BACK
};

const char* ToString(DistanceBand band);
const char* ToString(DirSector sector);

struct Perception
{
	void Update(const DirectX::XMFLOAT3& selfPos, float selfYaw, const DirectX::XMFLOAT3& targetPos);
	void ClearTarget();

	// band and sector
	float closeMax = 5.f;
	float midMax = 12.f;
	float frontHalfDeg = 45.f;
	float backStartDeg = 135.f;
	float distanceHysteresis = 0.5f;
	float angleHysteresis = 10.f;

	// attk options
	bool hasTarget = false;
	float distance = 0.f;
	float angleDeg = 0.f;

	DistanceBand band = DistanceBand::DISTANT;
	DirSector sector = DirSector::FRONT;

private:
	DistanceBand ClassifyBand(float dist) const;
	DirSector ClassifySector(float angle) const;
};
