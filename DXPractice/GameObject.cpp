#include "GameObject.h"

#include "AnimationSystem.h"
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

void GameObject::Init(ScriptSystem& inScriptSystem, AnimationSystem& inAnimationSystem, RenderSystem& inRenderSystem)
{
	scriptSystem = &inScriptSystem;
	animationSystem = &inAnimationSystem;
	renderSystem = &inRenderSystem;
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

std::vector<std::unique_ptr<MeshComponent>>& GameObject::GetMeshes()
{
	return meshes;
}

void GameObject::NotifyColliderEnter2D(const BoxCollider2D& other) const
{
	for (const auto comp : components)
	{
		comp.second->OnCollisionEnter2D(other);
	}
}

void GameObject::NotifyColliderLeave2D(const BoxCollider2D& other) const
{
	for (const auto comp : components)
	{
		comp.second->OnCollisionExit2D(other);
	}
}
