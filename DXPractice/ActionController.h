#pragma once
#include "IComponent.h"

class SkeletalAnimatorComponent;
struct ActionDef;

class ActionController : public IComponent
{
public:
	void OnAttach(GameContext& context) override;
	
	void Tick();
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

	bool IsFinished() const;
	
	bool IsComboOpen() const
	{
		return isComboOpen;
	}

	bool IsCancelOpen() const
	{
		return isCancelOpen;
	}

	bool DidCancelOpenThisFrame() const
	{
		return didCancelOpenThisFrame;
	}

private:
	SkeletalAnimatorComponent* animator = nullptr;
	const ActionDef* currAction = nullptr;

	bool isComboOpen = false;
	bool isCancelOpen = false;
	bool didCancelOpenThisFrame = false;
};

