#include "AnimatorComponent.h"

#include <stdexcept>

#include "GameObject.h"
#include "PixelShader.h"
#include "SpriteRendererComponent.h"
#include "TextureCache.h"

AnimatorComponent::AnimatorComponent(Renderer& renderer) : renderer(renderer), currAnimation(nullptr)
{
}

void AnimatorComponent::AddAnimation(const std::string& animName, const std::wstring& spritePath,
                                     const std::wstring& spriteJson)
{
	auto it = animations.find(animName);
	// stop if animation name already exists
	if (it != animations.end())
	{
		throw std::runtime_error("Animation name [" + animName + "] already used");
	}
	
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv = TextureCache::Load(renderer, spritePath);
	auto spriteRenderer = std::make_unique<SpriteRendererComponent>(renderer, srv.Get());
	spriteRenderer->owner = owner;
	auto spriteAnimator = std::make_unique<SpriteAnimatorComponent>(*spriteRenderer, spriteJson);
	spriteAnimator->owner = owner;

	animations[animName] = {std::move(spriteRenderer), std::move(spriteAnimator)};
}

AnimationEntry& AnimatorComponent::GetCurrAnimation() const
{
	return *currAnimation;
}

bool AnimatorComponent::SetCurrAnimation(const std::string& animName)
{
	auto it = animations.find(animName);

	if (it == animations.end())
	{
		return false;
	}
	currAnimation = &it->second;
	return true;
}

void AnimatorComponent::Update(float deltaTime)
{
	currAnimation->spriteRenderer->Update(deltaTime);
	currAnimation->spriteAnimator->Update(deltaTime);
	currAnimation->spriteRenderer->Bind(renderer);  // b1 UV, t0 texture, s0 sampler
}

void AnimatorComponent::Render()
{
	currAnimation->spriteRenderer->Render();
}

void AnimatorComponent::SetEnabled(bool inEnabled)
{
	enabled = inEnabled;
}
