#pragma once
#include <array>
#include <memory>

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

private:
	void RegisterStates();
	void ApplyPendingTransition();
	DragonState* GetState(StateID id) const;

private:
	static constexpr StateID noState = StateID::COUNT;

	std::array<std::unique_ptr<DragonState>, ToIndex(StateID::COUNT)> states;

	StateID currStateID = noState;
	StateID pendingStateID = noState;
	float timeInState = 0.f;

	SkeletalAnimatorComponent* animator = nullptr;
	TransformComponent* transform = nullptr;
};

