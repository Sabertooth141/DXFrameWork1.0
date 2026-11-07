#include "ModelReader.h"

#include <memory>
#include <assimp/postprocess.h>

namespace
{
	// convert assimp col major to dx row major
	DirectX::XMFLOAT4X4 ToRowMajor(const aiMatrix4x4& matrix)
	{
		DirectX::XMFLOAT4X4 out;
		DirectX::XMStoreFloat4x4(&out, DirectX::XMMatrixTranspose(DirectX::XMMATRIX(&matrix.a1)));
		return out;
	}

	DirectX::XMMATRIX ToXMMatrix(const aiMatrix4x4& inMatrix)
	{
		const DirectX::XMFLOAT4X4 out = ToRowMajor(inMatrix);
		return DirectX::XMLoadFloat4x4(&out);
	}

	// get global matrix in model space
	DirectX::XMMATRIX NodeGlobal(const aiNode* node)
	{
		DirectX::XMMATRIX global = ToXMMatrix(node->mTransformation);
		for (const aiNode* parent = node->mParent; parent != nullptr; parent = parent->mParent)
		{
			global = global * ToXMMatrix(parent->mTransformation);
		}
		return global;
	}

	bool SceneHasBones(const aiScene* scene)
	{
		for (unsigned i = 0; i < scene->mNumMeshes; i++)
		{
			if (scene->mMeshes[i]->HasBones())
			{
				return true;
			}
		}

		return false;
	}
}

ModelReader::ModelReader(Renderer& renderer, const std::string& path)
{
	Assimp::Importer importer;
	importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

	const aiScene* scene = importer.ReadFile(
		path,
		aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices |
		aiProcess_ConvertToLeftHanded |
		aiProcess_GenNormals |
		aiProcess_LimitBoneWeights // keep the 4 strongest influences per vertex
	);

	if (!scene || !scene->mRootNode)
	{
		std::string err = importer.GetErrorString();
		OutputDebugStringA(err.c_str());

		assert(false);
	}

	if (SceneHasBones(scene))
	{
		skeleton = std::make_shared<Skeleton>();
		BuildSkeletonNodes(scene->mRootNode, -1);
	}

	ProcessNode(renderer, scene->mRootNode, scene);

	// for debugging
	if (skeleton)
	{
		char msg[256];
		sprintf_s(msg, "[ModelReader] %s: %zu nodes, %zu bones (maxBones %u)\n",
		          path.c_str(), skeleton->nodes.size(), skeleton->bones.size(), maxBones);
		OutputDebugStringA(msg);
	}
}

const std::vector<MeshData>& ModelReader::GetMeshes() const
{
	return meshData;
}

const std::vector<SkinnedMeshData>& ModelReader::GetSkinnedMeshes() const
{
	return skinnedMeshDeta;
}

void ModelReader::ProcessNode(Renderer& renderer, aiNode* node, const aiScene* scene)
{
	for (unsigned i = 0; i < node->mNumMeshes; i++)
	{
		auto mesh = scene->mMeshes[node->mMeshes[i]];

		if (!skeleton)
		{
			meshData.push_back(ProcessMesh(mesh, scene));
		}
		else if (mesh->HasBones())
		{
			skinnedMeshDeta.push_back(ProcessSkinnedMesh(mesh, node));
		}
		else
		{
			OutputDebugStringA("[ModelReader] skipped a mesh without bones in a skinned model\n");
		}
	}

	for (unsigned i = 0; i < node->mNumChildren; i++)
	{
		ProcessNode(renderer, node->mChildren[i], scene);
	}
}

MeshData ModelReader::ProcessMesh(aiMesh* mesh, const aiScene* scene)
{
	MeshData result;

	// vertices
	for (unsigned i = 0; i < mesh->mNumVertices; i++)
	{
		Vertex v;

		v.pos = {
			mesh->mVertices[i].x,
			mesh->mVertices[i].y,
			mesh->mVertices[i].z
		};

		v.normal = {
			mesh->mNormals[i].x,
			mesh->mNormals[i].y,
			mesh->mNormals[i].z
		};

		if (mesh->mTextureCoords[0])
		{
			v.texcoord = {
				mesh->mTextureCoords[0][i].x,
				mesh->mTextureCoords[0][i].y
			};
		}
		else
		{
			v.texcoord = {0, 0};
		}

		result.vertices.push_back(v);
	}
	// texture
	if (mesh->mMaterialIndex < scene->mNumMaterials)
	{
	}

	// indices
	for (unsigned i = 0; i < mesh->mNumFaces; i++)
	{
		const aiFace& face = mesh->mFaces[i];
		for (unsigned j = 0; j < face.mNumIndices; j++)
		{
			result.indices.push_back(face.mIndices[j]);
		}
	}

	return result;
}

void ModelReader::BuildSkeletonNodes(const aiNode* node, int parentIndex)
{
	SkeletonNode skeletonNode;
	skeletonNode.name = node->mName.C_Str();
	skeletonNode.parent = parentIndex;
	skeletonNode.bindLocal = ToRowMajor(node->mTransformation);

	// per node TRS binding
	DirectX::XMVECTOR scale, rotation, translation;
	DirectX::XMMatrixDecompose(&scale, &rotation, &translation, DirectX::XMLoadFloat4x4(&skeletonNode.bindLocal));
	DirectX::XMStoreFloat3(&skeletonNode.bindTRS.translation, translation);
	DirectX::XMStoreFloat4(&skeletonNode.bindTRS.rotation, rotation);
	DirectX::XMStoreFloat3(&skeletonNode.bindTRS.scale, scale);

	skeleton->nodes.push_back(skeletonNode);
	const int index = static_cast<int>(skeleton->nodes.size() - 1);
	skeleton->nodeIndexByName.try_emplace(skeletonNode.name, index);

	// build parent first then move on
	for (unsigned i = 0; i < node->mNumChildren; i++)
	{
		BuildSkeletonNodes(node->mChildren[i], index);
	}
}

int ModelReader::FindOrAddBone(const aiBone* bone)
{
	const std::string name = bone->mName.C_Str();
	auto boneIndex = boneIndexByName.find(name);
	if (boneIndex != boneIndexByName.end())
	{
		return boneIndex->second;
	}

	auto nodeIndex = skeleton->nodeIndexByName.find(name);
	assert(nodeIndex != skeleton->nodeIndexByName.end() && "bone has no matching node");

	SkeletonBone newBone;
	newBone.node = nodeIndex->second;
	newBone.offset = ToRowMajor(bone->mOffsetMatrix);
	skeleton->bones.push_back(newBone);

	const int index = static_cast<int>(skeleton->bones.size() - 1);
	assert(
		index < static_cast<int>(maxBones) && "raise maxBones (Skeleton.h) and MAX_BONES (SkinnedVertexShader.hlsl)");

	boneIndexByName[name] = index;
	return index;
}

SkinnedMeshData ModelReader::ProcessSkinnedMesh(const aiMesh* mesh, const aiNode* node)
{
	SkinnedMeshData result;
	result.vertices.resize(mesh->mNumVertices);

	for (unsigned i = 0; i < mesh->mNumVertices; i++)
	{
		SkinnedVertex& vertex = result.vertices[i];
		vertex.pos = {mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z};
		vertex.normal = {mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z};
		vertex.texcoord = mesh->mTextureCoords[0]
			                  ? DirectX::XMFLOAT2{mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y}
			                  : DirectX::XMFLOAT2{0.f, 0.f};
	}

	// convert from bone -> vertices to vertices -> bones & info
	std::vector<uint8_t> usedSlotsPerVertex(mesh->mNumVertices, 0);
	for (unsigned bone = 0; bone < mesh->mNumBones; bone++)
	{
		const aiBone* currBone = mesh->mBones[bone];
		const int boneIndex = FindOrAddBone(currBone);

		for (unsigned weight = 0; weight < currBone->mNumWeights; weight++)
		{
			const aiVertexWeight& vertexWeight = currBone->mWeights[weight];
			uint8_t& slotCount = usedSlotsPerVertex[vertexWeight.mVertexId];

			// every vertex will only be influenced by up to 4 bones 
			if (vertexWeight.mWeight <= 0.f || slotCount >= 4)
			{
				continue;
			}

			SkinnedVertex& vertex = result.vertices[vertexWeight.mVertexId];
			vertex.boneIndices[slotCount] = static_cast<uint8_t>(boneIndex);
			vertex.boneWeights[slotCount] = vertexWeight.mWeight;
			slotCount += 1;
		}
	}

	// normalize weights per vertex - all must sum to 1
	unsigned unweightedCount = 0;
	for (SkinnedVertex& vertex : result.vertices)
	{
		const float sum = vertex.boneWeights[0] + vertex.boneWeights[1] + vertex.boneWeights[2] + vertex.boneWeights[3];
		// can only be 4 in total
		if (sum <= 0.f) // if vertex is not weighted
		{
			unweightedCount += 1;
			continue;
		}

		for (float& weight : vertex.boneWeights)
		{
			weight /= sum;
		}
	}

	// for debugging
	if (unweightedCount > 0)
	{
		char msg[128];
		sprintf_s(msg, "[ModelReader] %u vertices have no bone weights (will collapse to origin)\n", unweightedCount);
		OutputDebugStringA(msg);
	}

	for (unsigned i = 0; i < mesh->mNumFaces; i++)
	{
		const aiFace& face = mesh->mFaces[i];
		for (unsigned j = 0; j < face.mNumIndices; j++)
		{
			result.indices.push_back(face.mIndices[j]);
		}
	}

	// reset transform to identity
	if (!hasMeshSpace)
	{
		DirectX::XMStoreFloat4x4(&skeleton->meshSpaceInverse, DirectX::XMMatrixInverse(nullptr, NodeGlobal(node)));
		hasMeshSpace = true;
	}

	return result;
}
