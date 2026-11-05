#include "AnimationClipCache.h"

#include <assert.h>

#include "AnimationClip.h"

std::unordered_map<std::string, std::shared_ptr<const AnimationClip>> AnimationClipCache::cache;

std::shared_ptr<const AnimationClip> AnimationClipCache::LoadClip(const std::string& path, const Skeleton& skeleton)
{
	auto it = cache.find(path);
	if (it != cache.end())
	{
		assert(it->second->builtFor == &skeleton && "clip reused with another skeleton");
		return it->second;
	}

	auto clip= LoadAnimationClip(path, skeleton);
	cache.emplace(path, clip);
	return clip;

}

void AnimationClipCache::ClearCache()
{
	cache.clear();
}
