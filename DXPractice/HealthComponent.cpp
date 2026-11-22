#include "HealthComponent.h"

#include "GameObject.h"
#include "HitVolumeComponent.h"

HealthComponent::HealthComponent(float inMaxHip) : currHp(inMaxHip), maxHp(inMaxHip)
{
}

void HealthComponent::OnHitTaken(const HitEvent& event)
{
	if (!event.attacker)
	{
		return;
	}

	const HitVolumeComponent* attackerVolume = event.attacker->GetComponent<HitVolumeComponent>();
	if (!attackerVolume)
	{
		return;
	}

	// TODO: temp
	for (const HitGroup& group : attackerVolume->GetGroups())
	{
		if (group.name == event.hitboxGroup)
		{
			// TODO: basedamage 1.f TEMP change later
			ApplyDamage(group.damageScale * 1.f, group.staggerScale * 1.f, &event);
			return;
		}
	}
}

void HealthComponent::ApplyDamage(float damage, float stagger, const HitEvent* hitEvent)
{
	if (isDead)
	{
		return;
	}

	const float floorHp = std::min(minHp, currHp);
	currHp = std::max(currHp - damage, floorHp);

	if (onDamaged)
	{
		onDamaged(damage, stagger, hitEvent);
	}

	if (!isThreasholdCrossed && currHp <= maxHp * thresholdRatio)
	{
		isThreasholdCrossed = true;
		if (onThreasholdCrossed)
		{
			onThreasholdCrossed();
		}
	}

	if (currHp <= 0.f)
	{
		isDead = true;
		if (onDeath)
		{
			onDeath();
		}
	}
}

void HealthComponent::Reset()
{
	currHp = maxHp;
	isThreasholdCrossed = false;
	isDead = false;
}

void HealthComponent::SetHpRatio(float ratio)
{
	currHp = maxHp * std::clamp(ratio, 0.f, 1.f);
}
