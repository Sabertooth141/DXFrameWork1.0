#include "DragonAI.h"
#include "DragonStates.h"
#include "LocomotionComponent.h"
#include "SkeletalAnimatorComponent.h"
#include "TransformComponent.h"

void DragonAirRecoverState::Enter(DragonAI& ai)
{
	ai.GetAnimator().PlayClip(clipName);

	StrafeMotion& strafe = ai.GetStrafeMotion();
	strafe.BeginSlowdown();
	waitSec = std::max(recoverSec, strafe.GetSlowdownLeft());
}

void DragonAirRecoverState::Update(DragonAI& ai, float deltaTime)
{
	const float currStrafeSpeed = ai.GetStrafeMotion().Update(deltaTime);
	if (currStrafeSpeed > 0.f)
	{
		const float yaw = ai.GetTransform().GetRotation().y;
		ai.GetLocomotion().SetMove({sinf(yaw), 0.f, cosf(yaw)}, currStrafeSpeed);
	}

	if (ai.GetTimeInState() >= waitSec)
	{
		ai.RequestStateTransition(StateID::LAND);
	}
}
