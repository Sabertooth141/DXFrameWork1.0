#include "Rigidbody2DComponent.h"

#include <cmath>

#include "TransformComponent.h"

Rigidbody2DComponent::Rigidbody2DComponent(TransformComponent& inTransformComp, float inMass, bool inIsStatic) :
	transformComp(inTransformComp),
	mass(inMass),
	isStatic(inIsStatic)
{
	RefreshInv();
}

void Rigidbody2DComponent::AddForce(DirectX::XMFLOAT2 force)
{
	if (isStatic)
	{
		return;
	}

	accumulatedForce.x += force.x;
	accumulatedForce.y += force.y;
}

void Rigidbody2DComponent::AddTorque(float inTorque)
{
	if (invInertia <= 0)
	{
		return;
	}

	accumulatedTorque += inTorque;
}

void Rigidbody2DComponent::ApplyImpulse(DirectX::XMFLOAT2 impulse, DirectX::XMFLOAT2 contactVector)
{
	if (isStatic)
	{
		return;
	}

	velocity.x += impulse.x * invMass;
	velocity.y += impulse.y * invMass;

	angularVel += (contactVector.x * impulse.y - contactVector.y * impulse.x) * invInertia;
}

void Rigidbody2DComponent::Integrate(const float deltaTime)
{
	if (isStatic)
	{
		return;
	}

	// a = F / m
	velocity.x += accumulatedForce.x * invMass * deltaTime;
	velocity.y += accumulatedForce.y * invMass * deltaTime - gravity * deltaTime;

	// lin dampening
	const float damp = 1.0f / (1.0f + linearDampening * deltaTime);
	velocity.x *= damp;
	velocity.y *= damp;

	// apply angular velocity
	angularVel += accumulatedTorque * invInertia * deltaTime;

	// threshold to 0
	constexpr float sleepEpsilon = 1.f;
	if (std::abs(velocity.x) < sleepEpsilon)
	{
		velocity.x = 0;
	}

	if (std::abs(velocity.y) < sleepEpsilon)
	{
		velocity.y = 0;
	}

	// angular threshold to 0
	constexpr float angularSleepEpsilon = 0.10f;
	if (std::abs(angularVel) < angularSleepEpsilon)
	{
		angularVel = 0;
	}

	const float speedSq = velocity.x * velocity.x + velocity.y * velocity.y;
	if (speedSq < sleepEpsilon * sleepEpsilon && std::abs(angularVel) < angularSleepEpsilon)
	{
		sleepTimer += deltaTime;
		if (sleepTimer > 0.3f) { velocity = { 0, 0 }; angularVel = 0.f; }
	}
	else
	{
		sleepTimer = 0.f;
	}

	const DirectX::XMFLOAT3 currPos = transformComp.GetPosition();
	const DirectX::XMFLOAT3 currRot = transformComp.GetRotation();

	// update pos and rot
	transformComp.SetPosition({
		currPos.x + velocity.x * deltaTime,
		currPos.y + velocity.y * deltaTime,
		currPos.z
	});

	transformComp.SetRotation({
		currRot.x,
		currRot.y,
		currRot.z + angularVel * deltaTime
	});
}

void Rigidbody2DComponent::ClearAccumulator()
{
	accumulatedForce = {0, 0};
	accumulatedTorque = 0.f;
}

void Rigidbody2DComponent::RefreshInv()
{
	if (isStatic)
	{
		invMass = 0;
	}
	else
	{
		invMass = 1.f / mass;
	}
	
	if (isStatic || freezeRotation || inertia <= 0.f)
	{
		invInertia = 0.f;
	}
	else
	{
		invInertia = 1.f / inertia;
	}
}
