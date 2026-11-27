#pragma once
#include <array>
#include <cstdint>
#include <DirectXMath.h>

class Camera3D;
class Mouse;
class Keyboard;

enum class ActionButton : uint8_t
{
	NONE,
	ATTACK1,
	ATTACK2,
	ROLL
};

enum class InputButton : uint8_t
{
	ATTACK1,
	ATTACK2,
	ROLL,
	SPRINT,
	COUNT
};

enum class MouseButton : uint8_t
{
	NONE,
	LEFT,
	RIGHT
};

struct ButtonBinding
{
	unsigned char key = 0;
	MouseButton mouse = MouseButton::NONE;
};

struct PlayerIntent
{
	DirectX::XMFLOAT3 moveDir{};
	bool isSprinting = false;
	ActionButton pressedInput = ActionButton::NONE;

	bool HasMove() const
	{
		return moveDir.x != 0.f || moveDir.z != 0.f;
	}
};

class PlayerInputReader
{
public:
	PlayerInputReader();

	PlayerIntent ReadIntent(const Keyboard& keyboard, const Mouse& mouse, const Camera3D cam);
	void Bind(InputButton button, const ButtonBinding& binding);

private:
	static constexpr size_t buttonCount = static_cast<size_t>(InputButton::COUNT);

	static constexpr size_t Index(InputButton button)
	{
		return static_cast<size_t>(button);
	}

	bool IsDown(InputButton button, const Keyboard& keyboard, const Mouse& mouse) const;
	DirectX::XMFLOAT3 ReadMove(const Keyboard& keyboard, const Camera3D& cam) const;

	std::array<ButtonBinding, buttonCount> bindings{};
	std::array<bool, buttonCount> wasDown{};
};

class InputBuffer
{
public:
	void Update(ActionButton pressedInput, float deltaTime);
	void Clear();

	ActionButton Get() const
	{
		return bufferedAction;
	}

	bool HasAttack() const
	{
		return bufferedAction == ActionButton::ATTACK1 || bufferedAction == ActionButton::ATTACK2;
	}

public:
	float bufferSec = 0.f;

private:
	ActionButton bufferedAction = ActionButton::NONE;
	float ageSec = 0.f;
};
