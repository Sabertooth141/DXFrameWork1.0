#include "Scene.h"
#include "PrefabRegistry.h"
#include "SpriteVertex.h"
#include "App.h"

GameObject* Scene::Instantiate(const std::string& prefab, const DirectX::XMFLOAT3& pos)
{
    const PrefabFn* prefabFunction = PrefabRegistry::Instance().Find(prefab);
    if (!prefabFunction)
    {
        assert(false && "Scene::Instantiate: unknown prefab");
        return nullptr;
    }

    MeshData quad = MakeSpriteQuad();
    auto object = std::make_unique<GameObject>(
        quad.vertices, quad.indices, context);

    object->GetTransform()->SetPosition(pos);
    (*prefabFunction)(*object);

    GameObject* raw = object.get();
    pendingSpawn.push_back(std::move(object));
    return raw;
}

GameObject* Scene::Add2DObject()
{
    MeshData quad = MakeSpriteQuad();
    auto object = std::make_unique<GameObject>(quad.vertices, quad.indices, context);
    objects.push_back(std::move(object));

    return objects.back().get();
}

void Scene::Destroy(GameObject* go)
{
    pendingDestroy.push_back(go);
}

void Scene::Update(float deltaTime)
{
    for (auto& object : objects)
        object->Update(deltaTime);
}

void Scene::FlushPending()
{
    for (GameObject* object : pendingDestroy)
    {
        context.physicsSys.Unregister(object);
        context.scriptSys.Unregister(object);
        context.animationSys.Unregister(object->GetComponent<AnimatorComponent>());
        context.renderSys.Unregister(object);

        std::erase_if(objects, [object](const std::unique_ptr<GameObject>& p) { return p.get() == object; });
    }
    pendingDestroy.clear();

    for (auto& object : pendingSpawn)
        objects.push_back(std::move(object));
    pendingSpawn.clear();
}