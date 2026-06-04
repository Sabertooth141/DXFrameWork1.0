#include "GameObject.h"

#include "ModelReader.h"
#include "Material.h"

GameObject::GameObject(Renderer& renderer, const MaterialData& matData, const ModelReader& modelReader,
                       const std::wstring& vsPath, const std::wstring& psPath)
{
	TransformComponent& transformComp = AddComponent<TransformComponent>();
	AddComponent<MaterialComponent>(renderer, matData, vsPath, psPath);
	for (auto& meshData : modelReader.GetMeshes())
	{
		meshes.push_back(std::make_unique<MeshComponent>(renderer, meshData, transformComp));
	}
}

GameObject::GameObject(Renderer& renderer,
                       const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices,
                       const std::wstring& vsPath, const std::wstring& psPath)
{
	TransformComponent& transformComp = AddComponent<TransformComponent>();
	AddComponent<MaterialComponent>(renderer, MaterialData{}, vsPath, psPath);
	MeshData mesh = {vertices, indices};
	meshes.push_back(std::make_unique<MeshComponent>(renderer, mesh, transformComp));
}

GameObject::GameObject(Renderer& renderer)
{
	TransformComponent& transformComp = AddComponent<TransformComponent>();
}

void GameObject::Draw(Renderer& renderer)
{
	MaterialComponent* comp = GetComponent<MaterialComponent>();
	comp->Bind(renderer);
	for (auto& mesh : meshes)
	{
		mesh->Draw(renderer);
	}
}

void GameObject::Update(float deltaTime)
{
	for (auto& component : components)
	{
		component.second->Update(deltaTime);
	}
}

TransformComponent* GameObject::GetTransform()
{
	return GetComponent<TransformComponent>();
}
