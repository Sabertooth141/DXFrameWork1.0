#pragma once
#include <array>
#include <memory>

#include "DragonPerception.h"
#include "DragonState.h"
#include "MonoBehavior.h"

class SkeletalAnimatorComponent;
class TransformComponent;

class DragonAI : public MonoBehavior
{
public:
	void Start() override;
	void Update(float deltaTime) override;
	
	void RequestStateTransition(StateID nextStateID);

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

	const Perception& GetPerception() const
	{
		return perception;
	}

private:
	void RegisterStates();
	void ApplyPendingTransition();
	DragonState* GetState(StateID id) const;
	const GameObject* FindPlayer() const;

private:
	static constexpr StateID noState = StateID::COUNT;

	std::array<std::unique_ptr<DragonState>, ToIndex(StateID::COUNT)> states;

	StateID currStateID = noState;
	StateID pendingStateID = noState;
	float timeInState = 0.f;
	Perception perception;

	SkeletalAnimatorComponent* animator = nullptr;
	TransformComponent* transform = nullptr;
};

