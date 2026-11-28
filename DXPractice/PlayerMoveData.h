#pragma once
#include <string_view>
#include <vector>

struct PlayerMove
{
	const char* name;
	const char* action;
	const char* nextOnAttack = "";
	const char* nextOnAttack2 = "";
};

const std::vector<PlayerMove>& PlayerMoveData();

inline const PlayerMove* FindMove(const std::vector<PlayerMove>& moves, std::string_view name)
{
	for (const PlayerMove& move : moves)
	{
		if (name == move.name)
		{
			return &move;
		}
	}

	return nullptr;
}