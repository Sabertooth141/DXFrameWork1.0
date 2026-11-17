#include "LocomotionComponent.h"

#include "Collision3D.h"
#include "GameContext.h"
#include "GameObject.h"
#include "HitDetectionSystem.h"
#include "HitVolumeComponent.h"

class HitVolumeComponent;
struct HitShape;

void LocomotionComponent::SetMove(const DirectX::XMFLOAT3& dir, float speed)
{
	moveDir = {dir.x, 0.f, dir.z};
	moveSpeed = speed;
}

void LocomotionComponent::SetFacing(float targetYaw, float maxTurnRate)
{
	hasFacing = true;
	facingYaw = targetYaw;
	facingMaxTurnRate = maxTurnRate;
}

void LocomotionComponent::AddYaw(float deltaYaw)
{
	yawDelta += deltaYaw;
}

void LocomotionComponent::ApplyLocomotion(float deltaTime)
{
	ApplyMove(deltaTime);
	ApplyTurn(deltaTime);

	if (isPushedOut)
	{
		ResolvePushOut();
	}

	ClampToArena();
	ClearIntent();
}

void LocomotionComponent::ApplyMove(float deltaTime)
{
	if (moveSpeed <= 0.f || (moveDir.x == 0.f && moveDir.z == 0.f))
	{
		return;
	}

	owner->GetTransform()->Translate(
		{
			moveDir.x * moveSpeed * deltaTime, 0.f, moveDir.z * moveSpeed * deltaTime
		});
}

void LocomotionComponent::ApplyTurn(float deltaTime)
{
	if (!hasFacing && yawDelta == 0.f)
	{
		return;
	}

	TransformComponent* transform = owner->GetTransform();
	DirectX::XMFLOAT3 rot = transform->GetRotation();

	rot.y += yawDelta;

	if (hasFacing)
	{
		// shortest way around
		const float diff = remainderf(facingYaw - rot.y, DirectX::XM_2PI);
		const float maxStep = facingMaxTurnRate * deltaTime;
		rot.y += std::clamp(diff, -maxStep, maxStep);
	}

	rot.y = remainderf(rot.y, DirectX::XM_2PI);
	transform->SetRotation(rot);
}

void LocomotionComponent::ResolvePushOut()
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

void LocomotionComponent::ClampToArena()
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

Capsule3D LocomotionComponent::BuildBodyCapsule(const DirectX::XMFLOAT3& position) const
{
	return {
		{position.x, position.y - bodyHalfSegment, position.z},
		{position.x, position.y + bodyHalfSegment, position.z},
		bodyRadius
	};
}

void LocomotionComponent::ClearIntent()
{
	moveDir = {};
	moveSpeed = 0.f;
	hasFacing = false;
	yawDelta = 0.f;
}
