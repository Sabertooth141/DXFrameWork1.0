#pragma once
#include "MonoBehavior.h"

class HealthComponent;
class LocomotionComponent;
class Camera3D;

class PlayerController : public MonoBehavior
{
public:
	void Awake() override;
	void Start() override;
	void Update(float deltaTime) override;

private:
	DirectX::XMFLOAT3 HandleInput() const;

public:
	// intent amounts, applied through LocomotionComponent
	float moveSpeed = 5.f;
	float turnSpeed = 12.f;

private:
	const Camera3D* cam = nullptr;
	HealthComponent* health = nullptr;
	LocomotionComponent* locomotion = nullptr;
};