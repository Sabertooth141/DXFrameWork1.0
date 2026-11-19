#pragma once
#include <array>
#include <memory>

#include "DragonAttackData.h"
#include "DragonAttackSelector.h"
#include "DragonPerception.h"
#include "DragonState.h"
#include "MonoBehavior.h"

class LocomotionComponent;
class HealthComponent;
class SkeletalAnimatorComponent;
class TransformComponent;

class DragonAI : public MonoBehavior
{
public:
	void Start() override;
	void Update(float deltaTime) override;

	bool RequestStateTransition(StateID nextStateID);
	bool RequestAttack(int attkIndex);

	void OnHitDealt(const HitEvent& event) override;

#ifndef NDEBUG
	void DrawDebugUI() override;
	void DrawDebugGizmos(DebugRenderer& debugRenderer) const override;
#endif


	StateID GetCurrStateID() const
	{
		return currStateID;
	}

	float GetTimeInState() const
	{
		return timeInState;
	}

	SkeletalAnimatorComponent& GetAnimator() const
	{
		return *animator;
	}

	TransformComponent& GetTransform() const
	{
		return *transform;
	}

	LocomotionComponent& GetLocomotion() const
	{
		return *locomotion;
	}

	const Perception& GetPerception() const
	{
		return perception;
	}

	bool WasDamagedThisFrame() const
	{
		return wasDamaged;
	}

	bool IsInCombat() const
	{
		return isInCombat;
	}

	void SetInCombat(bool inCombat)
	{
		isInCombat = inCombat;
	}

	DragonAttackSelector& GetAttackSelector()
	{
		return attackSelector;
	}

	int GetCurrAttkIndex() const
	{
		return currAttkIndex;
	}

	bool HasAttackHit() const
	{
		return hasAttackHit;
	}

	void ClearAttackHit()
	{
		hasAttackHit = false;
	}

private:
	void RegisterStates();
	void BindHealthCallbacks();
	void CheckAnyStateTransition();
	bool ForceStateTransition(StateID nextStateID);
	bool CanCurrentBeInterrupted(InterruptStateKind kind) const;
	void ApplyPendingTransition();
	DragonState* GetState(StateID id) const;
	const GameObject* FindPlayer() const;

private:
	static constexpr auto noState = StateID::COUNT;

	std::array<std::unique_ptr<DragonState>, ToIndex(StateID::COUNT)> states;

	StateID currStateID = noState;
	StateID pendingStateID = noState;
	StateID lastIgnoredStateID = noState; // missing state logged once until next transition

	float timeInState = 0.f;
	bool isInCombat = false;

	Perception perception;
	DragonAttackSelector attackSelector{ DragonAttackData() };
	int currPhase = 1;

	// attack
	int currAttkIndex = -1;
	bool hasAttackHit = false;

	SkeletalAnimatorComponent* animator = nullptr;
	TransformComponent* transform = nullptr;
	HealthComponent* health = nullptr;
	LocomotionComponent* locomotion = nullptr;

	// flags by health callbacks
	bool isDeathPending = false;
	bool isPhaseChangePending = false;
	float staggerMeter = 0.f;
	bool wasDamaged = false;
};
