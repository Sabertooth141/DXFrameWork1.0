#include "ActionController.h"

#include "ActionData.h"
#include "GameObject.h"
#include "HitVolumeComponent.h"
#include "SkeletalAnimatorComponent.h"

namespace 
{
	constexpr auto comboWindowEvent = "ComboWindow";
	constexpr auto comboWindowEndEvent = "ComboWindowEnd";
	constexpr auto invulnOnEvent = "InvulnOn";
	constexpr auto invulnOffEvent = "InvulnOff";
}

void ActionController::OnAttach(GameContext& context)
{
	IComponent::OnAttach(context);

	animator = owner->GetComponent<SkeletalAnimatorComponent>();
	hitVolume = owner->GetComponent<HitVolumeComponent>();
}

void ActionController::Tick()
{
	didCancelOpenThisFrame = false;

	if (!currAction)
	{
		return;
	}

	if (hitVolume)
	{
		if (animator->WasMarkerFired(invulnOnEvent))
		{
			hitVolume->SetAllHurtboxEnabled(false);
			isInvuln = true;
		}
		if (animator->WasMarkerFired(invulnOffEvent))
		{
			hitVolume->SetAllHurtboxEnabled(true);
			isInvuln = false;
		}
	}

	if (!isCancelOpen && animator->WasMarkerFired(comboWindowEvent))
	{
		isCancelOpen = true;
		isComboOpen = true;
		didCancelOpenThisFrame = true;
	}

	if (animator->WasMarkerFired(comboWindowEndEvent))
	{
		isComboOpen = false;
	}
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
	if (isInvuln && hitVolume)
	{
		hitVolume->SetAllHurtboxEnabled(true);
	}

	currAction = nullptr;
	isComboOpen = false;
	isCancelOpen = false;
	didCancelOpenThisFrame = false;
	isInvuln = false;
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

bool ActionController::IsFinished() const
{
	return currAction && animator->IsFinished();
}
