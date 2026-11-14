#pragma once
#include "Collision3D.h"
#include "IComponent.h"
#include "MonoBehavior.h"

class Camera3D;

class PlayerController : public MonoBehavior
{
public:
	void Awake() override;
	void Update(float deltaTime) override;

private:
	DirectX::XMFLOAT3 HandleInput() const;
	void Move(float deltaTime);
	void TurnToward(const DirectX::XMFLOAT3& dir, float deltaTime);

	void ResolveCollisions();
	void ClampToArena();
	Capsule3D BuildBodyCapsule(const DirectX::XMFLOAT3& position) const;

public:
	// locomotion
	float moveSpeed = 5.f;
	float turnSpeed = 12.f;
	
	// collision
	float bodyRadius = 0.35f;
	float bodyHalfSegment = 0.55f;

	// clamp arena
	DirectX::XMFLOAT2 arenaCenterXZ{ 0.f, 0.f };
	float arenaRadius = 30.f;

private:
	const Camera3D* cam = nullptr;
};

