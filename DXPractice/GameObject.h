#pragma once
#include <typeindex>
#include <unordered_map>

#include "AnimationSystem.h"
#include "BoxCollider2D.h"
#include "MaterialComponent.h"
#include "MeshComponent.h"
#include "TransformComponent.h"
#include "MonoBehavior.h"
#include "ScriptSystem.h"
#include "RenderSystem.h"

class ModelReader;
class Renderer;

class GameObject
{
public:
	GameObject(Renderer& renderer, const MaterialData& matData, const ModelReader& modelReader,
	           const std::wstring& vsPath = L"VertexShader.cso", const std::wstring& psPath = L"PixelShader.cso");
	GameObject(Renderer& renderer, const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices,
	           const std::wstring& vsPath = L"VertexShader.cso", const std::wstring& psPath = L"PixelShader.cso");
	GameObject(Renderer& renderer);

	void Init(ScriptSystem& inScriptSystem, AnimationSystem& inAnimationSystem, RenderSystem& inRenderSystem);
	void Update(float deltaTime);

	template <typename T, typename... Args>
	T& AddComponent(Args&&... args)
	{
		std::unique_ptr<T> comp = std::make_unique<T>(std::forward<Args>(args)...);
		T& ref = *comp;
		ref.owner = this;

		if constexpr (std::is_base_of_v<MonoBehavior, T>)
		{
			scriptSystem->RegisterScript(&ref);
		}
		else if constexpr (std::is_base_of_v<AnimatorComponent, T>)
		{
			animationSystem->Register(&ref);
			renderSystem->Register(&ref);
		}

		components[typeid(T)] = std::move(comp);
		return ref;
	}

	template <typename T>
	T* GetComponent()
	{
		auto res = components.find(typeid(T));
		if (res != components.end())
		{
			return static_cast<T*>(res->second.get());
		}
		return nullptr;
	}

	void NotifyColliderEnter2D(const BoxCollider2D& other) const;
	void NotifyColliderLeave2D(const BoxCollider2D& other) const;

	TransformComponent* GetTransform();
	std::vector<std::unique_ptr<MeshComponent>>& GetMeshes();

private:
	std::unordered_map<std::type_index, std::unique_ptr<IComponent>> components;
	std::vector<std::unique_ptr<MeshComponent>> meshes;

	ScriptSystem* scriptSystem = nullptr;
	AnimationSystem* animationSystem = nullptr;
	RenderSystem* renderSystem = nullptr;
};
