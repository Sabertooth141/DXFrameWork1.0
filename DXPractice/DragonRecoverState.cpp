#include "DragonAI.h"
#include "DragonStates.h"
#include "SkeletalAnimatorComponent.h"

void DragonRecoverState::Enter(DragonAI& ai)
{
	ai.GetAnimator().PlayClip(clipName);

	waitSec = 0.f;
	const int attkIndex = ai.GetCurrAttkIndex();
	if (attkIndex >= 0)
	{
		const AttackOption& option = ai.GetAttackSelector().GetOption(attkIndex);
		if (ai.HasAttackHit())
		{
			waitSec = option.recoverSec;
		}
		else
		{
			waitSec = option.whiffRecoverSec;
		}

		waitSec *= ai.GetActionRecoverScale();
	}
}

void DragonRecoverState::Update(DragonAI& ai, float deltaTime)
{
	if (ai.GetTimeInState() >= waitSec)
	{
		ai.RequestStateTransition(StateID::IDLE);
	}
}