#pragma once
#include "DragonState.h"

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

public:
	float minTurnDeg = 45.f;
	float turn180FromDeg = 135.f;

private:
	bool isTurning = false;
	float warpScale = 1.f;
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
			return true;
		}

		return false;
	}

public:
	const char* clipName = "Roar";
};
