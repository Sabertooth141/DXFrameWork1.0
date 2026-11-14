#pragma once

#include "HitVolumeComponent.h"

class HitDetectionSystem
{
public:
	void Register(HitVolumeComponent* volume)
	{
		volumes.push_back(volume);
	}

	void Unregister(GameObject* object)
	{
		std::erase_if(volumes, [object](const HitVolumeComponent* volume)
			{
				return volume->owner == object;
			});
	}

	void Update(float deltaTime);

private:
	void DetectPair(HitVolumeComponent& attacker, HitVolumeComponent& victim, std::vector<HitEvent>& outHits) const;

private:
	std::vector<HitVolumeComponent*> volumes;
	float timeSec = 0.f; // game time
};

