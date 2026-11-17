#pragma once
#include "IComponent.h"

struct Capsule3D;

class LocomotionComponent : public IComponent
{
public:
	// xz for dir, m/s for speed
	void SetMove(const DirectX::XMFLOAT3& dir, float speed);

	void SetFacing(float targetYaw, float maxTurnRate);

	void AddYaw(float deltaYaw);

	void ApplyLocomotion(float deltaTime);

public:
	float bodyRadius = 0.35f;
	float bodyHalfSegment = 0.55f;
	bool isPushedOut = true;

	// arena clamps
	DirectX::XMFLOAT2 arenaCenterXZ{ 0.f, 0.f };
	float arenaRadius = 30.f;

private:
	void ApplyMove(float deltaTime);
	void ApplyTurn(float deltaTime);
	void ResolvePushOut();
	void ClampToArena();
	Capsule3D BuildBodyCapsule(const DirectX::XMFLOAT3& position) const;
	void ClearIntent();

private:
	DirectX::XMFLOAT3 moveDir{};
	float moveSpeed = 0.f;

	bool hasFacing = false;
	float facingYaw = 0.f;
	float facingMaxTurnRate = 0.f;

	float yawDelta = 0.f;
};
