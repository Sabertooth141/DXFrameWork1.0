#pragma once
#include <cstdint>

struct StrafeMotion
{
	enum class Phase : uint8_t
	{
		NONE,
		ACCEL,
		CRUISE,
		SLOWDOWN
	};

	void Begin(float inSpeed); 
	void BeginSlowdown();
	void Reset();
	float Update(float deltaTime); 

	float GetSpeed() const
	{
		return currSpeed;
	}

	Phase GetPhase() const
	{
		return phase;
	}

	float GetSlowdownLeft() const;

	float accelSec = 0.15f;
	float slowdownSec = 0.6f;

private:
	Phase phase = Phase::NONE;
	float cruiseSpeed = 0.f;
	float accelElapsed = 0.f;
	float slowdownStartSpeed = 0.f;
	float slowdownElapsed = 0.f;
	float currSpeed = 0.f;
};