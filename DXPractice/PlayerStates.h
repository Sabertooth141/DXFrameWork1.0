#pragma once
#include "PlayerState.h"

class PlayerLocomotionState : public PlayerState
{
public:
	void Update(PlayerController& player, const PlayerIntent& intent, float deltaTime) override;
};

// for player attacks | action options if added
class PlayerActionState : public PlayerState
{
public:
	void Update(PlayerController& player, const PlayerIntent& intent, float deltaTime) override;
};

class PlayerKnockdownState : public PlayerState
{
public:
	void Enter(PlayerController& player) override;
	void Update(PlayerController& player, const PlayerIntent& intent, float deltaTime) override;
};