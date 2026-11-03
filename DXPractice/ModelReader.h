#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>

#include "MeshData.h"
#include "Renderer.h"
#include "Skeleton.h"

class MeshComponent;

class ModelReader
{
public:
	ModelReader(Renderer& renderer, const std::string& path);
	const std::vector<MeshData>& GetMeshes() const;
	const std::vector<SkinnedMeshData>& GetSkinnedMeshes() const;

	bool HasSkeleton() const { return skeleton != nullptr; }
	
	std::shared_ptr<const Skeleton> GetSkeleton() const { return skeleton; }

private:
	void ProcessNode(Renderer& renderer, aiNode* node, const aiScene* scene);
	MeshData ProcessMesh(aiMesh* mesh, const aiScene* scene);

	void BuildSkeletonNodes(const aiNode* node, int parentIndex);
	SkinnedMeshData ProcessSkinnedMesh(const aiMesh* mesh, const aiNode* node);
	int FindOrAddBone(const aiBone* bone);

private:
	std::vector<MeshData> meshData;

	// for rigged models
	std::vector<SkinnedMeshData> skinnedMeshDeta;
	std::shared_ptr<Skeleton> skeleton;
	std::unordered_map<std::string, int> boneIndexByName;
	bool hasMeshSpace = false;
};
