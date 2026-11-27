#include "ActionController.h"

#include "ActionData.h"
#include "GameObject.h"
#include "SkeletalAnimatorComponent.h"

void ActionController::OnAttach(GameContext& context)
{
	IComponent::OnAttach(context);

	animator = owner->GetComponent<SkeletalAnimatorComponent>();
}

bool ActionController::Play(const ActionDef& action, float minFadeSec)
{
	Stop();

	if (!animator || !animator->PlayClip(action.clip, std::max(action.fadeInSec, minFadeSec)))
	{
		return false;
	}

	currAction = &action;
	return true;
}

void ActionController::Stop()
{
	currAction = nullptr;
}

float ActionController::GetRootMotionScale() const
{
	if (currAction)
	{
		return currAction->rootMotionScale;
	}
	else
	{
		return 1.f;
	}
}
