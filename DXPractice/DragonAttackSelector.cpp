#include "DragonAttackSelector.h"

#include "DragonAttackData.h"

DragonAttackSelector::DragonAttackSelector(const std::vector<AttackOption>& inAttackOptions) :
	attackOptions(inAttackOptions),
	cooldownLeft(inAttackOptions.size(), 0.f),
	rejects(inAttackOptions.size(), AttackReject::TARGET)
{
}

void DragonAttackSelector::TickCooldowns(float deltaTime)
{
	for (float& cdLeft : cooldownLeft)
	{
		cdLeft = (std::max(cdLeft - deltaTime, 0.f));
	}
}

void DragonAttackSelector::Evaluate(const Perception& perception, int phase)
{
	bestAttkIndex = -1;

	for (int i = 0; i < GetAttkCount(); i++)
	{
		rejects[i] = Check(i, perception, phase);
		if (rejects[i] != AttackReject::NONE)
		{
			continue;
		}

		if (bestAttkIndex < 0 || attackOptions[i].priority > attackOptions[bestAttkIndex].priority)
		{
			bestAttkIndex = i;
		}
	}
}

void DragonAttackSelector::StartCooldown(int index)
{
	cooldownLeft[index] = attackOptions[index].cooldownSec;
}

AttackReject DragonAttackSelector::Check(int index, const Perception& perception, int phase) const
{
	const AttackOption& option = attackOptions[index];

	if (!perception.hasTarget)
	{
		return AttackReject::TARGET;
	}

	if ((option.phases & ToPhaseBit(phase)) == 0)
	{
		return AttackReject::PHASE;
	}

	if (cooldownLeft[index] > 0)
	{
		return AttackReject::COOLDOWN;
	}

	if (perception.distance < option.minRange || perception.distance > option.maxRange)
	{
		return AttackReject::RANGE;
	}

	if ((option.sectors & ToSectorBit(perception.sector)) == 0)
	{
		return AttackReject::SECTOR;
	}

	return AttackReject::NONE;
}
