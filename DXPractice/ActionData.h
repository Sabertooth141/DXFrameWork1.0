#pragma once
#include <string>
#include <vector>

struct ActionDef
{
	const char* name;
	const char* clip;
	const char* nextOnAttack = "";
	const char* nextOnAttack2 = "";
	float fadeInSec = 0.1f;
	float rootMotionScale = 1.f;
};

const std::vector<ActionDef>& PlayerActionData();

inline const ActionDef* FindAction(const std::vector<ActionDef>& actions, std::string_view name)
{
	for (const ActionDef& action : actions)
	{
		if (name == action.name)
		{
			return &action;
		}
	}

	return nullptr;
}
