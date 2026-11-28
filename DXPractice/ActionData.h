#pragma once
#include <string>
#include <vector>

inline constexpr float useFadeTable = -1.f;

struct ActionDef
{
	const char* name;
	const char* clip;
	float fadeInSec = 0.1f; // val of useFadeTable = use animator fade table
	float rootMotionScale = 1.f;
};

const std::vector<ActionDef>& PlayerActionData();
const std::vector<ActionDef>& DragonActionData();

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
