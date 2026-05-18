#include "Model.h"

#include "Mesh.h"

Model::Model(Renderer& renderer, const std::string& path)
{
	Assimp::Importer importer;

	const aiScene* scene = importer.ReadFile(
		path,
		aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices |
		aiProcess_ConvertToLeftHanded |
		aiProcess_GenNormals
	);

	if (!scene || !scene->mRootNode)
	{
		std::string err = importer.GetErrorString();
		OutputDebugStringA(err.c_str());

		assert(false);
	}

	ProcessNode(renderer, scene->mRootNode, scene);
}

void Model::Draw(Renderer& renderer)
{
	for (auto& mesh : meshes)
	{
		mesh->Draw(renderer);
	}
}

void Model::ProcessNode(Renderer& renderer, aiNode* node, const aiScene* scene)
{
	for (unsigned i = 0; i < node->mNumMeshes; i++)
	{
		auto mesh = scene->mMeshes[node->mMeshes[i]];
		meshes.push_back(ProcessMesh(renderer, mesh));
	}

	for (unsigned i = 0; i < node->mNumChildren; i++)
	{
		ProcessNode(renderer, node->mChildren[i], scene);
	}
}

std::unique_ptr<Mesh> Model::ProcessMesh(Renderer& renderer, aiMesh* mesh)
{
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;

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

		vertices.push_back(v);
	}

	// indices
	for (unsigned i = 0; i < mesh->mNumFaces; i++)
	{
		const aiFace& face = mesh->mFaces[i];
		for (unsigned j = 0; j < face.mNumIndices; j++)
		{
			indices.push_back(face.mIndices[j]);
		}
	}

	return std::make_unique<Mesh>(renderer, vertices, indices);
}
