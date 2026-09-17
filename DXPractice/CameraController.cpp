#include "CameraController.h"

#include "Camera2D.h"
#include "GameObject.h"
#include "Scene.h"

void CameraController::Awake()
{
	MonoBehavior::Awake();
	camera = &owner->GetContext().camera;
	ownerRb = owner->GetComponent<Rigidbody2DComponent>();
}

void CameraController::LateUpdate(float deltaTime)
{
	float highestY = groundTopY;

	for (auto& obj : owner->GetContext().gameScene->GetObjects())
	{
		if (obj->GetTag() != ObjectTag::Block)
		{
			continue;
		}

		const auto* rb = obj->GetComponent<Rigidbody2DComponent>();
		if (rb && abs(rb->GetVelocity().y) > settleSpeed)
		{
			continue;
		}

		if (const auto* collider = obj->GetComponent<BoxCollider2D>())
		{
			highestY = std::max(highestY, collider->GetWorldAABB().max.y);
		}
	}

	float targetY = highestY + stackOffset;
	targetY = std::max(targetY, camera->GetPosition().y);

	const float t = 1.f - exp(-followLambda * deltaTime);
	DirectX::XMFLOAT2 camPos = camera->GetPosition();
	camPos.y += (targetY - camPos.y) * t;
	camera->SetPosition(camPos);

	DirectX::XMFLOAT3 playerPos = owner->GetTransform()->GetPosition();
	playerPos.y = camPos.y + playerOffset;
	owner->GetTransform()->SetPosition(playerPos);

	if (ownerRb)
	{
		ownerRb->SetVelocity({ ownerRb->GetVelocity().x, 0.f });
	}
}


