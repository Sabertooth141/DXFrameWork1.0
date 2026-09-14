#pragma once
#include <DirectXMath.h>

#include "IComponent.h"
class TransformComponent;

class Rigidbody2DComponent : public IComponent
{
public:
	Rigidbody2DComponent(TransformComponent& inTransformComp, float inMass, bool inIsStatic = false);

	void AddForce(DirectX::XMFLOAT2 force);
	void AddTorque(float inTorque);

	void SetGravity(const float inGrav)
	{
		gravity = inGrav;
	}

	void SetVelocity(const DirectX::XMFLOAT2 vel)
	{
		velocity = vel;
	}

	void SetAngularVel(const float inAngVel)
	{
		angularVel = inAngVel;
	}

	void SetRestitution(const float inRes)
	{
		restitution = inRes;
	}

	void SetIsStatic(const bool inStatic)
	{
		isStatic = inStatic;
	}

	DirectX::XMFLOAT2 GetVelocity() const
	{
		return velocity;
	}

	float GetAngularVel() const
	{
		return angularVel;
	}

	float GetInvMass() const
	{
		return invMass;
	}

	float GetRestitution() const
	{
		return restitution;
	}

	bool IsStatic() const
	{
		return isStatic;
	}

	TransformComponent& GetTransformComp() const
	{
		return transformComp;
	}

	// physics sys
	void Integrate(float deltaTime);
	void ClearAccumulator();

private:
	TransformComponent& transformComp;

	// property
	float gravity = 200.f;

	// velocities
	DirectX::XMFLOAT2 velocity = {0, 0};
	float angularVel = 0.f;

	// forces
	DirectX::XMFLOAT2 accumulatedForce = {0, 0};
	float accumulatedTorque = 0.f;

	// properties
	float invMass; // inverted mass for acceleration calculation F = ma
	bool isStatic;

	float restitution = 0.2f; // bounciness of body
	float linearDampening = 0.1f;
};
