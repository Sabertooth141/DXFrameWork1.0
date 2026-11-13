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

	void Update()
	{
		for (HitVolumeComponent* volume : volumes)
		{
			volume->ApplyAnimationEvents();
			volume->UpdateWorldShapes();
		}
	}

private:
	std::vector<HitVolumeComponent*> volumes;
};

