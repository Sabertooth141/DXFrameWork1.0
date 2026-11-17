#pragma once
#include <functional>

#include "IComponent.h"

class HealthComponent : public IComponent
{
public:
	explicit HealthComponent(float inMaxHip = 100.f);

	void OnHitTaken(const HitEvent& event) override;

	void ApplyDamage(float damage, float stagger);

	// for debug
	void Reset();
	void SetHpRatio(float ratio);

	float GetCurrHp() const
	{
		return currHp;
	}

	float GetMaxHp() const
	{
		return maxHp;
	}

	bool IsDead() const
	{
		return isDead;
	}

public:
	// for monster rage / special attk: when lower than this go to phase 2 / rage
	float thresholdRatio = 0.5f;

	// listerners 
	std::function<void(float damage, float stagger)> onDamaged;
	std::function<void()> onThreasholdCrossed;
	std::function<void()> onDeath;

private:
	float currHp = 0.f;
	float maxHp = 0.f;
	bool isDead = false;

	// rage flag
	bool isThreasholdCrossed = false;
};

