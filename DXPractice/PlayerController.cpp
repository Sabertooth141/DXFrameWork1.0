#include "PlayerController.h"

#include "GameContext.h"
#include "GameObject.h"
#include "HitDetectionSystem.h"
#include "HitVolumeComponent.h"
#include "Keyboard.h"

void PlayerController::Awake()
{
	MonoBehavior::Awake();

	cam = &owner->GetContext().cam3D;
}

void PlayerController::Update(float deltaTime)
{
	Move(deltaTime);
	ResolveCollisions();
	ClampToArena();
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

void PlayerController::Move(float deltaTime)
{
	const DirectX::XMFLOAT3 dir = HandleInput();

	if (dir.x == 0.f && dir.z == 0.f)
	{
		return;
	}

	owner->GetTransform()->Translate(
		{
			dir.x * moveSpeed * deltaTime, 0.f, dir.z * moveSpeed * deltaTime
		});

	TurnToward(dir, deltaTime);
}

void PlayerController::TurnToward(const DirectX::XMFLOAT3& dir, float deltaTime)
{
	TransformComponent* transform = owner->GetTransform();
	DirectX::XMFLOAT3 rot = transform->GetRotation();

	const float targetYaw = atan2f(dir.x, dir.z);

	// shortest way around
	const float diff = remainderf(targetYaw - rot.y, DirectX::XM_2PI);
	const float maxStep = turnSpeed * deltaTime;
	rot.y = remainderf(rot.y + std::clamp(diff, -maxStep, maxStep), DirectX::XM_2PI);

	transform->SetRotation(rot);
}

void PlayerController::ResolveCollisions()
{
	TransformComponent* transform = owner->GetTransform();
	DirectX::XMFLOAT3 position = transform->GetPosition();
	bool isPushed = false;

	for (const HitVolumeComponent* volume : owner->GetContext().hitDetectionSys.GetVolumes())
	{
		if (volume->owner == owner)
		{
			continue;
		}

		for (const HitShape& shape : volume->GetShapes())
		{
			if (shape.desc.role != HitVolumeRole::BODY || !shape.hasWorld)
			{
				continue;
			}

			DirectX::XMFLOAT3 pushDir{};
			if (ComputeHorizontalPushOut(BuildBodyCapsule(position), shape.world, pushDir))
			{
				position.x += pushDir.x;
				position.z += pushDir.z;
				isPushed = true;
			}
		}
	}

	if (isPushed)
	{
		transform->SetPosition(position);
	}
}

void PlayerController::ClampToArena()
{
	TransformComponent* transform = owner->GetTransform();
	DirectX::XMFLOAT3 position = transform->GetPosition();

	const float offsetX = position.x - arenaCenterXZ.x;
	const float offsetZ = position.z - arenaCenterXZ.y;
	const float distSq = offsetX * offsetX + offsetZ * offsetZ;
	const float maxDist = arenaRadius - bodyRadius;
	if (distSq <= maxDist * maxDist)
	{
		return;
	}

	const float scale = maxDist / sqrtf(distSq);
	position.x = arenaCenterXZ.x + offsetX * scale;
	position.z = arenaCenterXZ.y + offsetZ * scale;
	transform->SetPosition(position);
}

Capsule3D PlayerController::BuildBodyCapsule(const DirectX::XMFLOAT3& position) const
{
	return {
		{position.x, position.y - bodyHalfSegment, position.z},
		{position.x, position.y + bodyHalfSegment, position.z},
		bodyRadius
	};
}
