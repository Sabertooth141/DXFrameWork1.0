#pragma once
#include <algorithm>
#include <functional>
#include <string>
#include <unordered_map>

#include "Material.h"

class GameObject;

using PrefabFn = std::function<void(GameObject&)>;

enum class PrefabShape
{
	SPRITE,
	MODEL,
	EMPTY
};

struct PrefabDef
{
	PrefabShape shape = PrefabShape::SPRITE;
	std::string modelPath;
	MaterialData matData{};
	PrefabFn setupFunction;
}; 

class PrefabRegistry
{
public:
	static PrefabRegistry& Instance()
	{
		static PrefabRegistry instance;
		return instance;
	}

	void RegisterSprite(const std::string& name, const PrefabFn& fn)
	{
		PrefabDef def = {};
		def.setupFunction = fn;
		prefabs[name] = std::move(def);
	}

	void RegisterModel(const std::string& name, const std::string& modelPath, const MaterialData& matData, const PrefabFn& fn)
	{
		PrefabDef def = {};
		def.shape = PrefabShape::MODEL;
		def.modelPath = modelPath;
		def.matData = matData;
		def.setupFunction = fn;

		prefabs[name] = std::move(def);
	}
	void RegisterEmtpy(const std::string& name, const PrefabFn& fn)
	{
		PrefabDef def = {};
		def.shape = PrefabShape::EMPTY;
		def.setupFunction = fn;

		prefabs[name] = std::move(def);
	}

	const PrefabDef* Find(const std::string& name) const
	{
		auto it = prefabs.find(name);
		return it != prefabs.end() ? &it->second : nullptr;
	}

	std::vector<std::string> GetPrefabNames() const
	{
		std::vector<std::string> names;
		names.reserve(prefabs.size());
		for (const auto& [name, fn] : prefabs)
		{
			names.push_back(name);
		}
		std::ranges::sort(names);
		return names;
	}

private:
	PrefabRegistry() = default;
	std::unordered_map<std::string, PrefabDef> prefabs;
};

void RegisterPrefabs();
