#pragma once
#include <string>
#include <vector>

// Everything about a character's animations that is data, not code:
// which clips exist (and loop), the fade table, and the animation events.
// One file per character type, e.g. data/animsets/dragon.json

struct AnimClipDesc
{
	std::string name; // file stem in animDir, also the name used by PlayClip
	bool isLoop = false;
};

struct AnimFadeDesc
{
	std::string from = "*"; // "*" = any (AnimationFadeTable::any)
	std::string to = "*";
	float sec = 0.2f;
};

// seconds on the clip's own timeline
struct AnimEventDesc
{
	std::string clip;
	float timeSec = 0.f;
	std::string event;
	std::string param;
};

struct AnimSetDesc
{
	std::string animDir; // clip path = animDir + name + ".fbx"
	std::string initialClip;
	float defaultFadeSec = 0.2f;

	std::vector<AnimClipDesc> clips;
	std::vector<AnimFadeDesc> fades;
	std::vector<AnimEventDesc> events;
};

namespace AnimSetFile
{
	struct Result
	{
		bool ok = false;
		bool fileFound = false;
		std::string message; // for the editor
	};

	Result Load(const std::string& path, AnimSetDesc& out);
	Result Save(const std::string& path, const AnimSetDesc& desc);
}