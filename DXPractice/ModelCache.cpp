#include "ModelCache.h"

#include "ModelReader.h"

std::unordered_map<std::string, std::unique_ptr<const ModelReader>> ModelCache::cache;

const ModelReader& ModelCache::LoadModel(Renderer& renderer, const std::string& path)
{
	auto model = cache.find(path);

	if (model != cache.end())
	{
		return *model->second;
	}

	auto [pos, inserted] = cache.emplace(path, std::make_unique<ModelReader>(renderer, path));
	return *pos->second;
}

void ModelCache::ClearCache()
{
	cache.clear();
}
