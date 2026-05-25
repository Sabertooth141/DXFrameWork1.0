#pragma once
#include <memory>
#include <string>
#include <vector>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "MeshData.h"
#include "Renderer.h"

class MeshComponent;

class Model
{
public:
	Model(Renderer& renderer, const std::string& path);

private:
	void ProcessNode(Renderer& renderer, aiNode* node, const aiScene* scene);
	MeshData ProcessMesh(aiMesh* mesh);

private:
	std::vector<MeshData> meshData;
};

