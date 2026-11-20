#include "DragonAI.h"
#include "DragonStates.h"
#include "SkeletalAnimatorComponent.h"

void DragonRoarState::Enter(DragonAI& ai)
{
	isEnrage = ai.IsEnraging();
	ai.GetAnimator().PlayClip(clipName);
}

void DragonRoarState::Update(DragonAI& ai, float deltaTime)
{
	if (!ai.GetAnimator().IsFinished())
	{
		return;
	}

	if (isEnrage)
	{
		ai.EnterPhase2();
	}

	ai.SetInCombat(true);
	ai.RequestStateTransition(StateID::IDLE);
}