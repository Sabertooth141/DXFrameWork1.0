#include "InputSystem.h"

bool InputSystem::KeyIsPressed(unsigned char key) const
{
	return keyStates[key];
}

void InputSystem::OnKeyPressed(unsigned char key)
{
	keyStates[key] = true;
}

void InputSystem::OnKeyReleased(unsigned char key)
{
	keyStates[key] = false;
}
