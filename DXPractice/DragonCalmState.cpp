#include "DragonStates.h"

#include "DragonAI.h"
#include "SkeletalAnimatorComponent.h"

void DragonCalmState::Enter(DragonAI& ai)
{
	ai.GetAnimator().PlayClip(clipName);
}
