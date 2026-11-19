#include "DragonAI.h"
#include "DragonStates.h"
#include "SkeletalAnimatorComponent.h"

DragonSequenceClipState::DragonSequenceClipState(const char* inClipName, StateID inNextState) :
	clipName(inClipName),
	nextState(inNextState)
{
}

void DragonSequenceClipState::Enter(DragonAI& ai)
{
	isPlaying = ai.GetAnimator().PlayClip(clipName);
}

void DragonSequenceClipState::Update(DragonAI& ai, float deltaTime)
{
	if (!isPlaying || ai.GetAnimator().IsFinished())
	{
		ai.RequestStateTransition(nextState);
	}
}
