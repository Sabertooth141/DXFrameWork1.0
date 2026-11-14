#include "GameObject.h"

#include "AnimationSystem.h"
#include "App.h"
#include "ModelReader.h"
#include "Material.h"
#include "SkeletalAnimatorComponent.h"

GameObject::GameObject(const MaterialData& matData, const ModelReader& modelReader, GameContext& context,
                       const std::wstring& vsPath, const std::wstring& psPath) : GameObject(context)
{
	TransformComponent& transformComp = *GetTransform();

	if (modelReader.HasSkeleton())
	{
		AddComponent<MaterialComponent>(*renderer, matData, L"SkinnedVertexShader.cso", psPath, SkinnedVertex::GetInputLayout());
		for (auto& meshData : modelReader.GetSkinnedMeshes())
		{
			meshes.push_back(std::make_unique<MeshComponent>(*renderer, meshData, transformComp));
			MergeLocalBounds(meshData.vertices);
		}

		AddComponent<SkeletalAnimatorComponent>(*renderer, modelReader.GetSkeleton());
	}
	else
	{
		AddComponent<MaterialComponent>(*renderer, matData, vsPath, psPath);
		for (auto& meshData : modelReader.GetMeshes())
		{
			meshes.push_back(std::make_unique<MeshComponent>(*renderer, meshData, transformComp));
			MergeLocalBounds(meshData.vertices);
		}
	}

	AddComponent<MeshRendererComponent>();
}

GameObject::GameObject(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices,
                       GameContext& context,
                       const std::wstring& vsPath, const std::wstring& psPath) : GameObject(context)
{
	TransformComponent& transformComp = *GetTransform();
	AddComponent<MaterialComponent>(*renderer, MaterialData{}, vsPath, psPath);
	MeshData mesh = {vertices, indices};
	meshes.push_back(std::make_unique<MeshComponent>(*renderer, mesh, transformComp));
}

GameObject::GameObject(GameContext& context)
{
	this->context = &context;
	scriptSystem = &context.scriptSys;
	animationSystem = &context.animationSys;
	renderSystem = &context.renderSys;
	renderer = &context.renderer;
	physicsSystem = &context.physicsSys;
	TransformComponent& transformComp = AddComponent<TransformComponent>();
}

void GameObject::Update(float deltaTime)
{
}

TransformComponent* GameObject::GetTransform()
{
	return GetComponent<TransformComponent>();
}

std::vector<std::unique_ptr<MeshComponent>>& GameObject::GetMeshes()
{
	return meshes;
}

bool GameObject::GetWorldBounds(DirectX::BoundingOrientedBox& out) const
{
	if (!hasLocalBounds)
	{
		return false;
	}

	DirectX::BoundingOrientedBox localBox;
	DirectX::BoundingOrientedBox::CreateFromBoundingBox(localBox, localBounds);
	localBox.Transform(out, GetTransform()->GetMatrix());
	return true;
}

void GameObject::NotifyColliderEnter2D(const GameObject& other) const
{
	for (const auto& comp : components)
	{
		comp.second->OnCollisionEnter2D(other);
	}
}

void GameObject::NotifyColliderStay2D(const GameObject& other) const
{
	for (const auto& comp : components)
	{
		comp.second->OnCollisionStay2D(other);
	}
}

void GameObject::NotifyColliderLeave2D(const GameObject& other) const
{
	for (const auto& comp : components)
	{
		comp.second->OnCollisionExit2D(other);
	}
}

void GameObject::NotifyHitDealt(const HitEvent& event) const
{
	for (const auto& comp: components)
	{
		comp.second->OnHitDealt(event);
	}
}

void GameObject::NotifyHitTaken(const HitEvent& event) const
{
	for (const auto& comp : components)
	{
		comp.second->OnHitTaken(event);
	}
}
