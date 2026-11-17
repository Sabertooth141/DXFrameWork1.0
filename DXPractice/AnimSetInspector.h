#pragma once
#include <string>

class GameObject;
class AnimSetComponent;

// Inspector section for AnimSetComponent: clips, fades, animation events, save / revert
class AnimSetInspector
{
public:
	void Draw(GameObject& object, AnimSetComponent& animSet);

private:
	const GameObject* currObject = nullptr; // selection resets when this changes
	int selectedFade = -1;
	int selectedEvent = -1;
	std::string eventClipFilter; // empty = every clip
	std::string message;
	bool isMessageOk = true;
};
