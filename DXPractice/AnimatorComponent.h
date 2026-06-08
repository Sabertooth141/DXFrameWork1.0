#pragma once
#include <memory>
#include <string>
#include <unordered_map>

#include "IComponent.h"
#include "SpriteAnimatorComponent.h"
class GameObject;

struct AnimationEntry
{
	std::unique_ptr<SpriteRendererComponent> spriteRenderer;
	std::unique_ptr<SpriteAnimatorComponent> spriteAnimator;
};

class AnimatorComponent : public IComponent
{
public:
	AnimatorComponent(Renderer& renderer);
	void AddAnimation(const std::string& animName, const std::wstring& spritePath, const std::wstring& spriteJson);
	AnimationEntry& GetCurrAnimation() const;
	bool SetCurrAnimation(const std::string& animName);
	void Update(float deltaTime) override;
	
	bool IsEnabled() const { return enabled; }
	void SetEnabled(bool inEnabled);

private:
	Renderer& renderer;
	GameObject* parent = nullptr;
	std::unordered_map<std::string, AnimationEntry> animations;
	AnimationEntry* currAnimation;

	bool enabled = true;
};
