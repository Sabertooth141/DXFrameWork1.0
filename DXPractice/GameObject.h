#pragma once
#include <typeindex>
#include <unordered_map>
#include <DirectXCollision.h>
#include <memory>

#include "AnimationSystem.h"
#include "MaterialComponent.h"
#include "MeshComponent.h"
#include "ObjectTags.h"
#include "PhysicsSystem.h"
#include "RenderSystem.h"
#include "ScriptSystem.h"

class Rigidbody2DComponent;
class PhysicsSystem;
struct GameContext;
class ModelReader;
class Renderer;

class GameObject
{
public:
	// for 3d
	GameObject(const MaterialData& matData, const ModelReader& modelReader, GameContext& context,
	           const std::wstring& vsPath = L"VertexShader.cso", const std::wstring& psPath = L"PixelShader.cso");
	// for 2d
	GameObject(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, GameContext& context,
	           const std::wstring& vsPath = L"SpriteVertexShader.cso", const std::wstring& psPath = L"SpritePixelShader.cso");
	GameObject(GameContext& context);

	void Update(float deltaTime);

	template <typename T, typename... Args>
	T& AddComponent(Args&&... args)
	{
		static_assert(std::is_base_of_v<IComponent, T>, "T must derive from IComponent");

		std::unique_ptr<T> comp = std::make_unique<T>(std::forward<Args>(args)...);
		T& ref = *comp;
		ref.owner = this;
		ref.OnAttach(*context);

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

	template <typename T>
	const T* GetComponent() const
	{
		auto it = components.find(typeid(T));
		if (it == components.end()) return nullptr;
		return static_cast<const T*>(it->second.get());
	}

	const TransformComponent* GetTransform() const { return GetComponent<TransformComponent>(); }

	const std::string& GetName() const { return objName; }
	void SetName(const std::string& n) { objName = n; }
	std::string GetPrefabName() const { return prefabName; }
	void SetPrefabName(const std::string& n) { prefabName = n; }
	bool IsLayoutObject() const { return isLayoutObject; }
	void SetLayoutObject(bool b) { isLayoutObject = b; }

	void NotifyColliderEnter2D(const GameObject& other) const;
	void NotifyColliderStay2D(const GameObject& other) const;
	void NotifyColliderLeave2D(const GameObject& other) const;

	Renderer& GetRenderer() const
	{
		return *renderer;
	}

	GameContext& GetContext() const
	{
		return *context;
	}

	TransformComponent* GetTransform();
	std::vector<std::unique_ptr<MeshComponent>>& GetMeshes();

	ObjectTag GetTag() const
	{
		return tag;
	}

	void SetTag(ObjectTag inTag)
	{
		tag = inTag;
	}

	bool HasLocalBounds() const
	{
		return hasLocalBounds;
	}

	bool GetWorldBounds(DirectX::BoundingOrientedBox& out) const;

private:
	ObjectTag tag = ObjectTag::DEFAULT;
	std::unordered_map<std::type_index, std::unique_ptr<IComponent>> components;
	std::vector<std::unique_ptr<MeshComponent>> meshes;

	GameContext* context;
	Renderer* renderer = nullptr;
	ScriptSystem* scriptSystem = nullptr;
	AnimationSystem* animationSystem = nullptr;
	RenderSystem* renderSystem = nullptr;
	PhysicsSystem* physicsSystem = nullptr;

	std::string objName = "GameObject";
	std::string prefabName;
	bool isLayoutObject = false;

	// for 3d editor selecting
	DirectX::BoundingBox localBounds{};
	bool hasLocalBounds = false;
};
