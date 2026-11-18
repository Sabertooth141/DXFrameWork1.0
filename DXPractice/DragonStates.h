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
