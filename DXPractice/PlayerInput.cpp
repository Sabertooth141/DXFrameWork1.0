#include "PlayerInput.h"

#include "App.h"
#include "Camera3D.h"
#include "Keyboard.h"
#include "Mouse.h"

PlayerInputReader::PlayerInputReader()
{
	Bind(InputButton::ATTACK1, { .mouse = MouseButton::LEFT });
	Bind(InputButton::ATTACK2, { .mouse = MouseButton::RIGHT });
	Bind(InputButton::ROLL, { .key = VK_SPACE });
	Bind(InputButton::SPRINT, { .key = VK_SHIFT });
}

void PlayerInputReader::Bind(InputButton button, const ButtonBinding& binding)
{
	bindings[Index(button)] = binding;
}

PlayerIntent PlayerInputReader::ReadIntent(const Keyboard& keyboard, const Mouse& mouse, const Camera3D cam)
{
	PlayerIntent intent;
	intent.moveDir = ReadMove(keyboard, cam);

	std::array<bool, buttonCount> isDown{};
	for (size_t i = 0; i < buttonCount; i++)
	{
		isDown[i] = IsDown(static_cast<InputButton>(i), keyboard, mouse);
	}

	const auto isTriggered = [&](InputButton button)
	{
		const size_t i = Index(button);
		return isDown[i] && !wasDown[i];
	};

	intent.isSprinting = isDown[Index(InputButton::SPRINT)];
	// same-frame tie: first match wins (roll > attack2 > attack1)
	if (isTriggered(InputButton::ROLL))
	{
		intent.pressedInput = ActionButton::ROLL;
	}
	else if (isTriggered(InputButton::ATTACK2))
	{
		intent.pressedInput = ActionButton::ATTACK2;
	}
	else if (isTriggered(InputButton::ATTACK1))
	{
		intent.pressedInput = ActionButton::ATTACK1;
	}

	wasDown = isDown;
	return intent;
}

bool PlayerInputReader::IsDown(InputButton button, const Keyboard& keyboard, const Mouse& mouse) const
{
	const ButtonBinding& binding = bindings[Index(button)];

	if (binding.key != 0 && keyboard.KeyIsPressed(binding.key))
	{
		return true;
	}

	switch (binding.mouse)
	{
	case MouseButton::LEFT:
		return mouse.LeftPressed();
	case MouseButton::RIGHT:
		return mouse.RightPressed();
	case MouseButton::NONE:
		break;
	}

	// TODO: controller inputs
	return false;
}

DirectX::XMFLOAT3 PlayerInputReader::ReadMove(const Keyboard& keyboard, const Camera3D& cam) const
{
	float inputX = 0.f;
	float inputZ = 0.f;

	if (keyboard.KeyIsPressed('W'))
	{
		inputZ += 1.f;
	}
	if (keyboard.KeyIsPressed('S'))
	{
		inputZ -= 1.f;
	}
	if (keyboard.KeyIsPressed('D'))
	{
		inputX += 1.f;
	}
	if (keyboard.KeyIsPressed('A'))
	{
		inputX -= 1.f;
	}

	if (inputX == 0.f && inputZ == 0.f)
	{
		return {};
	}

	// camera relative W in towards camera
	const float sinYaw = sinf(cam.yaw);
	const float cosYaw = cosf(cam.yaw);
	const float dirX = sinYaw * inputZ + cosYaw * inputX;
	const float dirZ = cosYaw * inputZ - sinYaw * inputX;

	// normalize diagonals
	const float invLen = 1.f / sqrtf(dirX * dirX + dirZ * dirZ);
	return {dirX * invLen, 0.f, dirZ * invLen};
}

void InputBuffer::Update(ActionButton pressedInput, float deltaTime)
{
	if (pressedInput != ActionButton::NONE)
	{
		bufferedAction = pressedInput;
		ageSec = 0.f;
		return;
	}

	// input lasts buffersec
	if (bufferedAction != ActionButton::NONE)
	{
		ageSec += deltaTime;
		if (ageSec > bufferSec)
		{
			Clear();
		}
	}
}

void InputBuffer::Clear()
{
	bufferedAction = ActionButton::NONE;
	ageSec = 0.f;
}
