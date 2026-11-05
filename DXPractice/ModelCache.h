#pragma once
#include <memory>
#include <string>
#include <unordered_map>

class Renderer;
class ModelReader;

class ModelCache
{
public:
	static const ModelReader& LoadModel(Renderer& renderer, const std::string& path);
	static void ClearCache();
	
private:
	static std::unordered_map<std::string, std::unique_ptr<const ModelReader>> cache;
};

