#include "DragonAI.h"
#include "DragonStates.h"
#include "SkeletalAnimatorComponent.h"

void DragonDeadState::Enter(DragonAI& ai)
{
	ai.GetAnimator().PlayClip(clipName);

	if (HitVolumeComponent* hitVolume = ai.GetHitVolume())
	{
		hitVolume->SetAllHurtboxEnabled(false);
	}

	ai.GetStrafeMotion().Reset();
}

void DragonDeadState::Update(DragonAI& ai, float deltaTime)
{
	DragonState::Update(ai, deltaTime);
}