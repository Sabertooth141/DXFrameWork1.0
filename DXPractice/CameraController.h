#pragma once
#include "MonoBehavior.h"
#include "WindowSettings.h"

class Rigidbody2DComponent;
class Camera2D;

class CameraController : public MonoBehavior
{
public:
	void Awake() override;
	void LateUpdate(float deltaTime) override;

	void SetGroundTop(float y)
	{
		groundTopY = y;
	}

private:
	Camera2D* camera = nullptr;
	Rigidbody2DComponent* ownerRb = nullptr;

	float groundTopY = -300.f;
	float stackOffset = WIN_HEIGHT * 0.25f;
	float playerOffset = WIN_HEIGHT * 0.35f;
	float followLambda = 6.f;
	float settleSpeed = 2.f;
};

