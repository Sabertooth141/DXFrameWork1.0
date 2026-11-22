#include "CameraController.h"

#include <complex>

#include "GameContext.h"
#include "GameObject.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "Scene.h"

void CameraController::Awake()
{
	MonoBehavior::Awake();

	cam = &owner->GetContext().cam3D;
}

void CameraController::Start()
{
	MonoBehavior::Start();

	yaw = cam->yaw;
	orbitPitch = std::clamp(-cam->pitch, DirectX::XMConvertToRadians(minPitchDeg), DirectX::XMConvertToRadians(maxPitchDeg));

	smoothedTarget = GetTargetPoint();
}

void CameraController::LateUpdate(float deltaTime)
{
	HandleOrbitInput();
	HandleZoomInput();

	// snap to target
	if (keyboard->KeyIsPressed('Q'))
	{
		StartSnapToTarget();
	}
	UpdateSnap(deltaTime);

	// lerp
	const DirectX::XMFLOAT3 target = GetTargetPoint();
	const float t = 1.f - std::exp(-followSpeed * deltaTime);
	smoothedTarget.x += (target.x - smoothedTarget.x) * t;
	smoothedTarget.y += (target.y - smoothedTarget.y) * t;
	smoothedTarget.z += (target.z - smoothedTarget.z) * t;

	ApplyToCamera();
}

void CameraController::HandleOrbitInput()
{
	const Mouse::RawDelta delta = mouse->ConsumeRawDelta();

	const bool isOrbit = !keyboard->KeyIsPressed(VK_MENU);
	owner->GetContext().wantsCursorlock = isOrbit;
	if (!isOrbit || (delta.x == 0 && delta.y == 0))
	{
		return;
	}

	isSnapping = false;

	yaw = remainderf(yaw + static_cast<float>(delta.x) * lookSens, DirectX::XM_2PI);

	orbitPitch = std::clamp(orbitPitch + static_cast<float>(delta.y) * lookSens,
	                        DirectX::XMConvertToRadians(minPitchDeg), DirectX::XMConvertToRadians(maxPitchDeg));
}

void CameraController::HandleZoomInput()
{
	while (const std::optional<Mouse::Event> event = mouse->Read())
	{
		if (event->GetType() == Mouse::Event::Type::WheelUp)
		{
			distance -= zoomStep;
		}
		else if (event->GetType() == Mouse::Event::Type::WheelDown)
		{
			distance += zoomStep;
		}
	}

	distance = std::clamp(distance, minDistance, maxDistance);
}

DirectX::XMFLOAT3 CameraController::GetTargetPoint() const
{
	const DirectX::XMFLOAT3 pos = owner->GetTransform()->GetPosition();
	return {pos.x, pos.y + heightOffset, pos.z};
}

void CameraController::ApplyToCamera() const
{
	cam->yaw = yaw;
	cam->pitch = -orbitPitch;

	// slides camera right for offset
	const DirectX::XMVECTOR lookTarget = DirectX::XMVectorAdd(DirectX::XMLoadFloat3(&smoothedTarget),
	                                                          DirectX::XMVectorScale(cam->GetCamRight(), shoulderOffset));


	const DirectX::XMVECTOR forward = cam->GetCamForward();
	const DirectX::XMVECTOR pos = DirectX::XMVectorSubtract(lookTarget, DirectX::XMVectorScale(forward, distance));
	DirectX::XMStoreFloat3(&cam->pos, pos);
}

const GameObject* CameraController::FindTarget() const
{
	const Scene* currScene = owner->GetContext().currScene;
	if (!currScene)
	{
		return nullptr;
	}

	for (const auto& object : currScene->GetObjects())
	{
		if (object->GetTag() == ObjectTag::ENEMY && !currScene->IsPendingDestroy(object.get()))
		{
			return object.get();
		}
	}

	return nullptr;
}

void CameraController::StartSnapToTarget()
{
	const GameObject* target = FindTarget();
	if (!target)
	{
		return;
	}

	const DirectX::XMFLOAT3 from = owner->GetTransform()->GetPosition();
	const DirectX::XMFLOAT3 to = target->GetTransform()->GetPosition();
	const float dirX = to.x - from.x;
	const float dirZ = to.z - from.z;
	const float horizontalSq = dirX * dirX + dirZ * dirZ;

	if (horizontalSq < 1e-4f)
	{
		return;
	}
	snapYaw = atan2f(dirX, dirZ);

	const DirectX::XMFLOAT3 targetPoint = GetTargetPoint();
	const float dirY = (to.y + snapLookHeight) - targetPoint.y;
	// down -> positive y
	snapPitch = std::clamp(-atan2f(dirY, sqrtf(horizontalSq)),
	                       DirectX::XMConvertToRadians(minPitchDeg), DirectX::XMConvertToRadians(maxPitchDeg));
	isSnapping = true;
}

void CameraController::UpdateSnap(float deltaTime)
{
	if (!isSnapping)
	{
		return;
	}

	const float yawDiff = remainderf(snapYaw - yaw, DirectX::XM_2PI);
	const float pitchDiff = snapPitch - orbitPitch;

	const float endRad = DirectX::XMConvertToRadians(snapEndDeg);
	if (fabsf(yawDiff) < endRad && fabsf(pitchDiff) < endRad)
	{
		yaw = snapYaw;
		orbitPitch = snapPitch;
		isSnapping = false;
		return;
	}

	// lerp to target
	const float t = 1.f - std::exp(-snapSpeed * deltaTime);
	yaw = remainderf(yaw + yawDiff * t, DirectX::XM_2PI);
	orbitPitch += pitchDiff * t;
}

#ifndef NDEBUG
void CameraController::FrameDragon()
{
	const Scene* scene = owner->GetContext().currScene;
	if (!scene)
	{
		return;
	}

	for (const auto& object : scene->GetObjects())
	{
		if (object->GetPrefabName() != "dragon")
		{
			continue;
		}

		// look from behind the player toward the dragon
		const DirectX::XMFLOAT3 from = owner->GetTransform()->GetPosition();
		const DirectX::XMFLOAT3 to = object->GetTransform()->GetPosition();
		const float dirX = to.x - from.x;
		const float dirZ = to.z - from.z;
		if (dirX * dirX + dirZ * dirZ < 1e-4f)
		{
			return;
		}

		yaw = atan2f(dirX, dirZ);
		orbitPitch = DirectX::XMConvertToRadians(framePitchDeg);
		return;
	}
}
#endif
