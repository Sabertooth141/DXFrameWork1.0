#include "MonoBehavior.h"

void MonoBehavior::SetInput(Keyboard& inKeyboard, Mouse& inMouse)
{
	keyboard = &inKeyboard;
	mouse = &inMouse;
}
