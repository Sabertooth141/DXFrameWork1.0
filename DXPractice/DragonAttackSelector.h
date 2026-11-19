#pragma once
#include <cstdint>
#include <vector>

struct Perception;
struct AttackOption;

// first filter that failed for each attackoption || order of evaluation
enum class AttackReject : uint8_t
{
	NONE,
	TARGET, // if no target
	PHASE, // if not in phase
	COOLDOWN, // if under cooldown
	RANGE, // if not in range 
	SECTOR // in not in dir sector
};

// evaluate once per frame per option
class DragonAttackSelector
{
public:
	explicit DragonAttackSelector(const std::vector<AttackOption>& inAttackOptions);

	void TickCooldowns(float deltaTime);
	void Evaluate(const Perception& perception, int phase);
	void StartCooldown(int index);
	
	// nothing valid = -1
	int GetBestAttkIndex() const
	{
		return bestAttkIndex;
	}

	int GetAttkCount() const
	{
		return static_cast<int>(attackOptions.size());
	}

	const AttackOption& GetOption(int index) const
	{
		return attackOptions[index];
	}

	float GetCooldownLeft(int index) const
	{
		return cooldownLeft[index];
	}

	AttackReject GetReject(int index) const
	{
		return rejects[index];
	}

private:
	AttackReject Check(int index, const Perception& perception, int phase) const;

private:
	const std::vector<AttackOption>& attackOptions;
	std::vector<float> cooldownLeft; // cooldown left per option
	std::vector<AttackReject> rejects; // reject reason per option
	int bestAttkIndex = -1;
};

