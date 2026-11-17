#pragma once
#include <string>

class GameObject;
class HitVolumeComponent;

// Inspector section for HitVolumeComponent: shape list, groups, bone binding, save / revert
class HitVolumeInspector
{
public:
	void Draw(GameObject& object, HitVolumeComponent& volume);

private:
	const GameObject* currObject = nullptr; // selection resets when this changes
	int selectedShape = -1;
	char boneFilter[64] = {};
	std::string message;
	bool isMessageOk = true;
};
