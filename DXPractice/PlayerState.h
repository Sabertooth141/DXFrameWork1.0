#pragma once
#include <cstdint>

struct PlayerIntent;
class PlayerController;

enum class PlayerStateID : uint8_t
{
	LOCOMOTION,
	ACTION,
	KNOCKDOWN,
	COUNT
};

inline const char* ToString(PlayerStateID id)
{
	switch (id)
	{
	case PlayerStateID::LOCOMOTION:
		return "LOCOMOTION";
	case PlayerStateID::ACTION:
		return "ACTION";
	case PlayerStateID::KNOCKDOWN:
		return "KNOCKDOWN";
	default:
		return "NONE";
	}
}

class PlayerState
{
public:
	virtual ~PlayerState() = default;

	virtual void Enter(PlayerController& player)
	{
	}

	virtual void Update(PlayerController& player, const PlayerIntent& intent, float deltaTime)
	{
	}

	virtual void Exit(PlayerController& player)
	{
	}

#ifndef NDEBUG
	virtual void Validate(const PlayerController& player) const
	{
	}
#endif
};
