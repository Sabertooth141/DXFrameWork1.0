#pragma once
#include "DragonState.h"

struct AttackOption;

inline constexpr auto trackEndEvent = "TrackEnd";

class DragonCalmState : public DragonState
{
public:
	void Enter(DragonAI& ai) override;
	void Update(DragonAI& ai, float deltaTime) override;

#ifndef NDEBUG
	void DrawDebug(const DragonAI& ai, DebugRenderer& debugRenderer) const override;
#endif

public:
	const char* clipName = "Idle1";
	float aggroRadius = 15.f;
};


class DragonTurnState : public DragonState
{
public:
	void Enter(DragonAI& ai) override;
	void Update(DragonAI& ai, float deltaTime) override;

#ifndef NDEBUG
	void DrawDebug(const DragonAI& ai, DebugRenderer& debugRenderer) const override;
#endif

private:
	StateID GetNextState(const DragonAI& ai) const;
	float PredictAngleDeg(const DragonAI& ai, float leadSec) const;

public:
	float minTurnDeg = 45.f;
	float turn180FromDeg = 135.f;
	float leadScale = 0.75f; // leads to player future pos when turning
	float minWarpScale = 0.5f;
	float maxWarpScale = 1.5f;

private:
	bool isTurning = false;
	float finalWarpScale = 1.f;
	float targetYaw = 0.f;
};


class DragonRoarState : public DragonState
{
public:
	void Enter(DragonAI& ai) override;
	void Update(DragonAI& ai, float deltaTime) override;

	bool CanBeInterrupted(InterruptStateKind kind) const override
	{
		if (kind == InterruptStateKind::STAGGER)
		{
			return !isEnrage;
		}

		return false;
	}

public:
	const char* clipName = "Roar";

private:
	bool isEnrage = false;
};


class DragonIdleState : public DragonState
{
public:
	void Enter(DragonAI& ai) override;
	void Update(DragonAI& ai, float deltaTime) override;

public:
	const char* clipName = "Idle1";
	float trackTurnRate = 40.f; // in deg, small tracks to player dir
};

class DragonApproachState : public DragonState
{
public:
	void Enter(DragonAI& ai) override;
	void Update(DragonAI& ai, float deltaTime) override;

#ifndef NDEBUG
	void DrawDebug(const DragonAI& ai, DebugRenderer& debugRenderer) const override;
#endif

public:
	const char* clipName = "WalkF";
	float trackTurnRate = 40.f; // in deg, small tracks to player dir

	float walkStrideSpeed = 147.7f; // measured in fbx; model units/s

private:
	float GetWalkSpeed(const DragonAI& ai) const;
};

class DragonAttackState : public DragonState
{
public:
	void Enter(DragonAI& ai) override;
	void Update(DragonAI& ai, float deltaTime) override;
	void Exit(DragonAI& ai) override;

	bool CanBeInterrupted(InterruptStateKind kind) const override
	{
		return kind != InterruptStateKind::PHASE_CHANGE;
	}

#ifndef NDEBUG
	void DrawDebug(const DragonAI& ai, DebugRenderer& debugRenderer) const override;
#endif

private:
	const AttackOption* option = nullptr;
	bool isPlaying = false;
	bool isTracking = false;
};

class DragonRecoverState : public DragonState
{
public:
	void Enter(DragonAI& ai) override;
	void Update(DragonAI& ai, float deltaTime) override;

	bool CanBeInterrupted(InterruptStateKind kind) const override
	{
		return kind != InterruptStateKind::PHASE_CHANGE;
	}

public:
	const char* clipName = "Idle1";

private:
	float waitSec = 0.f;
};

class DragonAirRecoverState : public DragonState
{
public:
	void Enter(DragonAI& ai) override;
	void Update(DragonAI& ai, float deltaTime) override;

	bool CanBeInterrupted(InterruptStateKind kind) const override
	{
		return kind != InterruptStateKind::PHASE_CHANGE;
	}

public:
	const char* clipName = "Idle3";
	float recoverSec = 1.f;

private:
	float waitSec = 0.f;
};

class DragonSequenceClipState : public DragonState
{
public:
	DragonSequenceClipState(const char* inClipName, StateID inNextState);

	void Enter(DragonAI& ai) override;
	void Update(DragonAI& ai, float deltaTime) override;

	bool CanBeInterrupted(InterruptStateKind kind) const override
	{
		return false;
	}

private:
	const char* clipName;
	StateID nextState;
	bool isPlaying = false;
};

class DragonAirAimState : public DragonState
{
public:
	void Enter(DragonAI& ai) override;
	void Update(DragonAI& ai, float deltaTime) override;

	bool CanBeInterrupted(InterruptStateKind kind) const override
	{
		return false;
	}

#ifndef NDEBUG
	void DrawDebug(const DragonAI& ai, DebugRenderer& debugRenderer) const override;
#endif

public:
	const char* clipName = "FlyB";
	float retreatSpeed = 20.f;
	float retreatDistance = 10.f;
	float turnRate = 90.f;
	float aimToleranceDeg = 10.f;
	float minAimSec = 0.8f;
	float maxAimSec = 3.f;
};

class DragonAirAttackState : public DragonState
{
public:
	void Enter(DragonAI& ai) override;
	void Update(DragonAI& ai, float deltaTime) override;

	bool CanBeInterrupted(InterruptStateKind kind) const override
	{
		return false;
	}

#ifndef NDEBUG
	void DrawDebug(const DragonAI& ai, DebugRenderer& debugRenderer) const override;
#endif

public:
	const char* clipName = "FlyAttk1";
	float trackSpeedDeg = 60.f;
	float strafeSpeed = 20.f;

private:
	bool isPlaying = false;
	bool isTracking = false;
};

class DragonDeadState : public DragonState
{
public:
	void Enter(DragonAI& ai) override;
	void Update(DragonAI& ai, float deltaTime) override;

	bool CanBeInterrupted(InterruptStateKind kind) const override
	{
		return false;
	}

public:
	const char* clipName = "Death1";
};