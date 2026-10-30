#pragma once
#include <memory>
#include <string>
#include <vector>
#include <DirectXMath.h>

#include "GameObject.h"
#include "GameContext.h"

struct PrefabDef;
class TextRenderer;

class Scene
{
public:
	explicit Scene(GameContext& inCtx) : context(inCtx)
	{
	}

	virtual ~Scene() = default;
	Scene& operator=(const Scene&) = delete;

	GameObject* Instantiate(const std::string& prefab, const DirectX::XMFLOAT3& pos);
	GameObject* InstantiateAsLayout(const std::string& prefab, const DirectX::XMFLOAT3& pos);
	GameObject* Add2DObject();
	GameObject* Add3DObject(const std::string& modelPath, const MaterialData& matData);
	GameObject* AddEmptyObject();

	std::unique_ptr<GameObject> CreateObjectShell(const PrefabDef& def);

	void Destroy(GameObject* object);

	virtual void OnEnter()
	{
	}

	virtual void OnUpdate(float deltaTime)
	{
	}

	virtual void OnExit()
	{
	}

	virtual void OnDrawUI(TextRenderer& text)
	{
	}

	void Update(float deltaTime);
	void Clear(); // scene uninit
	void FlushPending();
	bool IsPendingDestroy(const GameObject* obj) const;

	const std::vector<std::unique_ptr<GameObject>>& GetObjects() const
	{
		return objects;
	}

	std::vector<std::unique_ptr<GameObject>>& GetObjects()
	{
		return objects;
	}

	PhysicsSystem& GetPhysics() const
	{
		return context.physicsSys;
	}

protected:
	GameContext& context;

private:
	void UnregisterFromSys(GameObject* obj);

	std::vector<std::unique_ptr<GameObject>> objects;
	std::vector<std::unique_ptr<GameObject>> pendingSpawn;
	std::vector<GameObject*> pendingDestroy;
	bool isClearing = false;
};
