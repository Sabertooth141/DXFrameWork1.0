#include "DragonAI.h"
#include "DragonStates.h"
#include "LocomotionComponent.h"
#include "SkeletalAnimatorComponent.h"
#include "TransformComponent.h"

void DragonIdleState::Enter(DragonAI& ai)
{
	ai.GetAnimator().PlayClip(clipName);
}

void DragonIdleState::Update(DragonAI& ai, float deltaTime)
{
	const Perception& perception = ai.GetPerception();
	if (!perception.hasTarget)
	{
		return;
	}

	// too far on the side to track - go to turn
	if (perception.sector != DirSector::FRONT)
	{
		ai.RequestStateTransition(StateID::TURN);
		return;
	}

	const float yaw = ai.GetTransform().GetRotation().y;
	ai.GetLocomotion().SetFacing(yaw + DirectX::XMConvertToRadians(perception.angleDeg),
	                             DirectX::XMConvertToRadians(trackTurnRate));

	// too far away from target ; approach
	if (perception.band != DistanceBand::CLOSE)
	{
		ai.RequestStateTransition(StateID::APPROACH);
	}
}
