#pragma once
#include <DirectXMath.h>
#include <string>

class GameObject;

struct HitEvent
{
	GameObject* attacker = nullptr;
	GameObject* victim = nullptr;

	std::string hitboxGroup;
	std::string hurtboxName;
	DirectX::XMFLOAT3 contactPoint{};
	unsigned activation = 0;
};
