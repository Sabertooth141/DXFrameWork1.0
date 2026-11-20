#include "PlayerController.h"

#include "GameContext.h"
#include "GameObject.h"
#include "HealthComponent.h"
#include "Keyboard.h"
#include "LocomotionComponent.h"
#include "SkeletalAnimatorComponent.h"
#include "TextRenderer.h"

void PlayerController::Awake()
{
	MonoBehavior::Awake();

	cam = &owner->GetContext().cam3D;
}

void PlayerController::Start()
{
	MonoBehavior::Start();

	locomotion = owner->GetComponent<LocomotionComponent>();
	animator = owner->GetComponent<SkeletalAnimatorComponent>();
	if (!locomotion)
	{
#ifndef NDEBUG
		textRenderer->Log("[PlayerController] %s: no LocomotionComponent, controller disabled", owner->GetName().c_str());
#endif
		SetEnabled(false);
		return;
	}

	health = owner->GetComponent<HealthComponent>();
	if (health)
	{
		health->onDeath = [this]()
		{
			health->Reset();
		};
	}
}

void PlayerController::Update(float deltaTime)
{
	const DirectX::XMFLOAT3 dir = HandleInput();

	if (dir.x != 0.f || dir.z != 0.f)
	{
		locomotion->SetMove(dir, moveSpeed);
		locomotion->SetFacing(atan2f(dir.x, dir.z), turnSpeed);
	}

	if (animator)
	{
		locomotion->AddMove(animator->GetRootMoveDelta());
	}

	locomotion->ApplyLocomotion(deltaTime);
}

DirectX::XMFLOAT3 PlayerController::HandleInput() const
{
	float inputX = 0.f;
	float inputZ = 0.f;
	if (keyboard->KeyIsPressed('W'))
	{
		inputZ += 1.f;
	}
	if (keyboard->KeyIsPressed('S'))
	{
		inputZ -= 1.f;
	}
	if (keyboard->KeyIsPressed('D'))
	{
		inputX += 1.f;
	}
	if (keyboard->KeyIsPressed('A'))
	{
		inputX -= 1.f;
	}

	if (inputX == 0.f && inputZ == 0.f)
	{
		return {};
	}

	const float sinYaw = sinf(cam->yaw);
	const float cosYaw = cosf(cam->yaw);
	const float dirX = sinYaw * inputZ + cosYaw * inputX;
	const float dirZ = cosYaw * inputZ - sinYaw * inputX;

	// normalize 
	const float invLen = 1.f / sqrtf(dirX * dirX + dirZ * dirZ);
	return {dirX * invLen, 0.f, dirZ * invLen};
}