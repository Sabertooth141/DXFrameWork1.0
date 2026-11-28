#pragma once
#include <DirectXMath.h>

#include "PlayerInput.h"
#include "PlayerState.h"

struct PlayerMove;
inline constexpr float instantTurnRate = 80.f;
inline constexpr auto comboWindowEvent = "ComboWindow";

class PlayerLocomotionState : public PlayerState
{
public:
	void Update(PlayerController& player, const PlayerIntent& intent, float deltaTime) override;

private:
	void PlayLocomotionClip(PlayerController& player, const char* clipName) const;

public:
	// intent amounts, applied through LocomotionComponent
	float runSpeed = 3.7f;
	float sprintSpeed = 6.2f;
	float turnSpeed = 12.f;
	float locomotionFadeSec = 0.15f;

	// TODO: TEMP
	const char* idleClip = "IdleCombat";
	const char* runClip = "RunLoop";
	const char* sprintClip = "RunFastLoop";
};

// for player attacks | action options if added
class PlayerActionState : public PlayerState
{
public:
	void Enter(PlayerController& player) override;
	void Update(PlayerController& player, const PlayerIntent& intent, float deltaTime) override;
	void Exit(PlayerController& player) override;

#ifndef NDEBUG
	void Validate(const PlayerController& player) const override;
#endif

private:
	const char* PickMove(ActionButton button, const PlayerMove* fromMove) const;
	void StartMove(PlayerController& player, const char* moveName, const DirectX::XMFLOAT3& dir, float minFadeSec);

public:
	float lateChainFadeSec = 0.1f;

	// TODO: openers for each combo - object to change later
	const char* attackMove = "Combo1Attack1";
	const char* attack2Move = "Combo2Attack1";
	const char* rollMove = "RollF";

private:
	InputBuffer inputBuffer;
	const PlayerMove* currMove = nullptr;
};

class PlayerKnockdownState : public PlayerState
{
public:
	void Enter(PlayerController& player) override;
	void Update(PlayerController& player, const PlayerIntent& intent, float deltaTime) override;
	void Exit(PlayerController& player) override;

public:
	const char* knockdownClip = "Death_RM";
	const char* getUpClip = "GetUpB";

	float knockdownStartSec = 1.f; // skip Death_RM's stumble, start at the fall
	float knockdownFadeSec = 0.1f;
	float knockdownAirSec = 0.45f; // Death_RM's fall takes about this long
	float knockdownAirHeight = 0.4f;
	float knockdownPushDistance = 1.7f; // added to the clip's own ~0.8 m

private:
	DirectX::XMFLOAT3 knockdownBackDir{};
	float knockdownTimer = 0.f;
	float knockdownGroundY = 0.f;

	bool isGettingUp = false;
	bool isGetUpCancelOpen = false;
};
