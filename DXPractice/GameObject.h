#pragma once
#include <typeindex>
#include <unordered_map>

#include "MaterialComponent.h"
#include "MeshComponent.h"
#include "TransformComponent.h"

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
	void Draw(Renderer& renderer);
	void Update(float deltaTime);

	template <typename T, typename... Args>
	T& AddComponent(Args&&... args)
	{
		std::unique_ptr<T> comp = std::make_unique<T>(std::forward<Args>(args)...);
		T& ref = *comp;
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
};
