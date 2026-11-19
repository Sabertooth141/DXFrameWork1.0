#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "DragonPerception.h"

// bitmask conversion for attk options
constexpr uint8_t ToSectorBit(DirSector sector)
{
	return static_cast<uint8_t>(1u << static_cast<uint8_t>(sector));
}

constexpr uint8_t sectorFront = ToSectorBit(DirSector::FRONT);
constexpr uint8_t sectorLeft = ToSectorBit(DirSector::LEFT);
constexpr uint8_t sectorRight = ToSectorBit(DirSector::RIGHT);
constexpr uint8_t sectorBack = ToSectorBit(DirSector::BACK);

constexpr uint8_t ToPhaseBit(int phase)
{
	return static_cast<uint8_t>(1u << (phase - 1));
}

constexpr uint8_t phaseAll = ToPhaseBit(1) | ToPhaseBit(2);

struct AttackOption
{
	std::string name;
	std::string clip;

	int priority = 0; // higher = more priority
	float cooldownSec = 0.f;

	// filters
	float minRange = 0.f;
	float maxRange = 0.f;
	uint8_t sectors = sectorFront;
	uint8_t phases = phaseAll;

	float trackSpeedDeg = 0.f;
	float recoverSec = 1.f;
	float whiffRecoverSec = 1.5f;
	std::string hitboxGroup;
};

const std::vector<AttackOption>& DragonAttackData();
