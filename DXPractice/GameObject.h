#pragma once
#include <typeindex>
#include <unordered_map>

#include "AnimationSystem.h"
#include "MaterialComponent.h"
#include "MeshComponent.h"
#include "TransformComponent.h"
#include "MonoBehavior.h"
#include "ScriptSystem.h"

class ScriptSystem;
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

	void Init(ScriptSystem& inScriptSystem, AnimationSystem& inAnimationSystem);

	void Draw(Renderer& renderer);
	void Update(float deltaTime);

	template <typename T, typename... Args>
	T& AddComponent(Args&&... args)
	{
		std::unique_ptr<T> comp = std::make_unique<T>(std::forward<Args>(args)...);
		T& ref = *comp;

		if constexpr (std::is_base_of_v<MonoBehavior, T>)
		{
			ref.owner = this;
			scriptSystem->RegisterScript(ref);
		}
		else if constexpr (std::is_base_of_v<AnimatorComponent, T>)
		{
			animationSystem->Register(&ref);
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

	TransformComponent* GetTransform();

private:
	std::unordered_map<std::type_index, std::unique_ptr<IComponent>> components;
	std::vector<std::unique_ptr<MeshComponent>> meshes;

	ScriptSystem* scriptSystem = nullptr;
	AnimationSystem* animationSystem = nullptr;
};
