#pragma once
#include <memory>
#include <string>
#include <unordered_map>

struct Skeleton;
struct AnimationClip;

class AnimationClipCache
{
public:
	static std::shared_ptr<const AnimationClip> LoadClip(const std::string& path, const Skeleton& skeleton);
	static void ClearCache();

private:
	static std::unordered_map<std::string, std::shared_ptr<const AnimationClip>> cache;
};

