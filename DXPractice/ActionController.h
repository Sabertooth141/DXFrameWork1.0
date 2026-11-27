#pragma once
#include "IComponent.h"

class SkeletalAnimatorComponent;
struct ActionDef;

class ActionController : public IComponent
{
public:
	void OnAttach(GameContext& context) override;
	
	// ends curr clip - if clip missing / nothing active = false
	bool Play(const ActionDef& action, float minFadeSec = 0.f);
	// end or interrupt
	void Stop();

	const ActionDef* GetCurrAction() const
	{
		return currAction;
	}

	bool IsActive() const
	{
		return currAction != nullptr;
	}

	float GetRootMotionScale() const;

private:
	SkeletalAnimatorComponent* animator = nullptr;
	const ActionDef* currAction = nullptr;
};

