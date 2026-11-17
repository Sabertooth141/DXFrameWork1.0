#pragma once
#include "DragonState.h"

class DragonCalmState : public DragonState
{
public:
	void Enter(DragonAI& ai) override;

public:
	const char* clipName = "Idle1";
};

