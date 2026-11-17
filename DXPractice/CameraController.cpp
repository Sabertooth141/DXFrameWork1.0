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

#ifndef NDEBUG
	if (keyboard->KeyIsTriggered(VK_F3))
	{
		FrameDragon();
	}
#endif

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

	bool isOrbit = mouse->RightPressed();
	owner->GetContext().wantsCursorlock = isOrbit;
	if (!isOrbit)
	{
		return;
	}

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

	const DirectX::XMVECTOR forward = cam->GetCamForward();
	const DirectX::XMVECTOR pos = DirectX::XMVectorSubtract(
		DirectX::XMLoadFloat3(&smoothedTarget), DirectX::XMVectorScale(forward, distance));
	DirectX::XMStoreFloat3(&cam->pos, pos);
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
