#include "GameObject.h"

#include "ModelReader.h"
#include "Material.h"

GameObject::GameObject(Renderer& renderer, const MaterialData& matData, const ModelReader& modelReader) :
transformComp({0, 0, 0}, {0, 0, 0}, {1, 1, 1}),
materialComp(renderer, matData)
{
	for (auto& meshData : modelReader.GetMeshes())
	{
		meshes.push_back(std::make_unique<MeshComponent>(renderer, meshData, transformComp));
	}
}

GameObject::GameObject(Renderer& renderer, const std::vector<Vertex> vertices, const std::vector<unsigned int> indices) :
	transformComp({ 0, 0, 0 }, { 0, 0, 0 }, { 1, 1, 1 }),
	materialComp(renderer, {})
{
	MeshData mesh = {vertices, indices};
	meshes.push_back(std::make_unique<MeshComponent>(renderer, mesh, transformComp));
}

GameObject::GameObject(Renderer& renderer) :
	transformComp({ 0, 0, 0 }, { 0, 0, 0 }, { 1, 1, 1 }),
	materialComp(renderer, {})
{
}

void GameObject::Draw(Renderer& renderer)
{
	materialComp.Bind(renderer);
	for (auto& mesh : meshes)
	{
		mesh->Draw(renderer);
	}
}

void GameObject::Update(float deltaTime)
{
}

TransformComponent* GameObject::GetTransform()
{
	return &transformComp;
}
