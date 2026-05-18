#pragma once
#include <memory>
#include <string>
#include <vector>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Renderer.h"
#include "Mesh.h"

class Model
{
public:
	Model(Renderer& renderer, const std::string& path);
	void Draw(Renderer& renderer);

private:
	void ProcessNode(Renderer& renderer, aiNode* node, const aiScene* scene);
	std::unique_ptr<Mesh> ProcessMesh(Renderer& renderer, aiMesh* mesh);

private:
	std::vector<std::unique_ptr<Mesh>> meshes;
};

