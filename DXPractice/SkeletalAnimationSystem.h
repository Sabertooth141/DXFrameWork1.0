#pragma once
#include "SkeletalAnimatorComponent.h"

class SkeletalAnimationSystem
{
public:
	void Register(SkeletalAnimatorComponent* animator)
	{
		animators.push_back(animator);
	}

	void Unregister(GameObject* object)
	{
		std::erase_if(animators, [object](const SkeletalAnimatorComponent* animator)
		{
			return animator->owner == object;
		});
	}

	void Update(float deltaTime)
	{
		for (auto* animator : animators)
		{
			animator->Update(deltaTime);
		}
	}

private:
	std::vector<SkeletalAnimatorComponent*> animators;
};
