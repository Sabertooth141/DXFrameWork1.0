#pragma once

class InputSystem
{
public:
	bool KeyIsPressed(unsigned char key) const;
	void OnKeyPressed(unsigned char key);
	void OnKeyReleased(unsigned char key);

private:
	bool keyStates[256] = {};
};

