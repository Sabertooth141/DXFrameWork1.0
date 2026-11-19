#include "DragonStrafeMotion.h"

#include <algorithm>

void StrafeMotion::Begin(float inSpeed)
{
	phase = Phase::ACCEL;
	cruiseSpeed = inSpeed;
	accelElapsed = 0.f;
}

void StrafeMotion::BeginSlowdown()
{
	if (phase != Phase::ACCEL && phase != Phase::CRUISE)
	{
		return;
	}

	phase = Phase::SLOWDOWN;
	slowdownStartSpeed = currSpeed;
	slowdownElapsed = 0.f;
}

void StrafeMotion::Reset()
{
	phase = Phase::NONE;
	currSpeed = 0.f;
}

float StrafeMotion::Update(float deltaTime)
{
	switch (phase)
	{
	case Phase::NONE:
		currSpeed = 0.f;
		break;

	case Phase::ACCEL:
		{
			accelElapsed += deltaTime;
			const float s = accelSec > 0.f ? std::min(accelElapsed / accelSec, 1.f) : 1.f;

			const float inv = 1.f - s;
			currSpeed = cruiseSpeed * (1.f - inv * inv * inv);

			if (s >= 1.f)
			{
				phase = Phase::CRUISE;
				currSpeed = cruiseSpeed;
			}
			break;
		}

	case Phase::CRUISE:
		currSpeed = cruiseSpeed;
		break;

	case Phase::SLOWDOWN:
		{
			slowdownElapsed += deltaTime;
			const float s = slowdownSec > 0.f ? std::min(slowdownElapsed / slowdownSec, 1.f) : 1.f;

			const float smooth = s * s * (3.f - 2.f * s);
			currSpeed = slowdownStartSpeed * (1.f - smooth);

			if (s >= 1.f)
			{
				phase = Phase::NONE;
				currSpeed = 0.f;
			}
			break;
		}
	}

	return currSpeed;
}

float StrafeMotion::GetSlowdownLeft() const
{
	if (phase != Phase::SLOWDOWN)
	{
		return 0.f;
	}

	return std::max(slowdownSec - slowdownElapsed, 0.f);
}
