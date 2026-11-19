#include "DragonAI.h"
#include "DragonStates.h"
#include "SkeletalAnimatorComponent.h"

void DragonRoarState::Enter(DragonAI& ai)
{
	ai.GetAnimator().PlayClip(clipName);
}

void DragonRoarState::Update(DragonAI& ai, float deltaTime)
{
	if (!ai.GetAnimator().IsFinished())
	{
		return;
	}

	ai.SetInCombat(true);
	ai.RequestStateTransition(StateID::IDLE);
}