#pragma once
#include <string>
#include <unordered_map>

class AnimationFadeTable
{
public:
	static constexpr const char* any = "*";

	explicit AnimationFadeTable(float inDefaultSec = 0.2f) : defaultSec(inDefaultSec)
	{
		
	}

	void SetFade(const std::string& fromClipName, const std::string& toClipName, float fadeTimeSec)
	{
		fadeTable[CreateKey(fromClipName, toClipName)] = fadeTimeSec;
	}

	float GetFadeTime(const std::string& fromClipName, const std::string& toClipName) const
	{
		if (const float* time = Find(fromClipName, toClipName))
		{
			return *time;
		}

		if (const float* time = Find(any, toClipName))
		{
			return *time;
		}

		if (const float* time = Find(fromClipName, any))
		{
			return *time;
		}

		return defaultSec;
	}

private:
	static std::string CreateKey(const std::string& fromClipName, const std::string& toClipName)
	{
		return fromClipName + '\n' + toClipName;
	}

	const float* Find(const std::string& fromClipName, const std::string& totoClipName) const
	{
		auto it = fadeTable.find(CreateKey(fromClipName, totoClipName));
		return it != fadeTable.end() ? &it->second : nullptr;
	}

private:
	std::unordered_map<std::string, float> fadeTable;
	float defaultSec;
};
