#include "MonoBehavior.h"

#include "GameObject.h"
#include "Scene.h"
#include "SceneManager.h"

void MonoBehavior::OnAttach(GameContext& context)
{
	IComponent::OnAttach(context);

	SetInput(context.keyboard, context.mouse);
	SetDebugRenderer(context);
	context.scriptSys.RegisterScript(this);
}

void MonoBehavior::ChangeScene(const std::string& sceneName, float exitTime, float enterTime) const
{
	owner->GetContext().sceneManager->RequestChange(sceneName, exitTime, enterTime);
}

void MonoBehavior::SetInput(Keyboard& inKeyboard, Mouse& inMouse)
{
	keyboard = &inKeyboard;
	mouse = &inMouse;
}

void MonoBehavior::SetDebugRenderer(const GameContext& context)
{
	textRenderer = &context.textRenderer;
}

GameObject* MonoBehavior::Instantiate(const std::string& prefab, const DirectX::XMFLOAT3& pos) const
{
	return owner->GetContext().currScene->Instantiate(prefab, pos);
}

void MonoBehavior::Destroy(GameObject* object) const
{
	owner->GetContext().currScene->Destroy(object);
}

void MonoBehavior::DestroySelf() const
{
	Destroy(owner);
}
