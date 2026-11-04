#pragma once
#include <vector>

#include "AnimatorComponent.h"

class AnimationSystem
{
public:
	void Register(AnimatorComponent* animator)
	{
		animators.push_back(animator);
	}

	void Unregister(GameObject* object)
	{
		std::erase_if(animators, [object](const AnimatorComponent* animator)
		{
			return animator->owner == object;
		});
	}

	void Update(float deltaTime)
	{
		for (auto* anim : animators)
		{
			if (anim->IsEnabled())
			{
				anim->Update(deltaTime);
			}
		}
	}

private:
	std::vector<AnimatorComponent*> animators;
};
