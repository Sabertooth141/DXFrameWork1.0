#include "Rigidbody2DComponent.h"

#include <cmath>

#include "TransformComponent.h"

Rigidbody2DComponent::Rigidbody2DComponent(TransformComponent& inTransformComp, float inMass, bool inIsStatic) :
	transformComp(inTransformComp),
	invMass(inIsStatic ? 0 : 1.0f / inMass),
	isStatic(inIsStatic)
{
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
	if (isStatic)
	{
		return;
	}

	accumulatedTorque += inTorque;
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

	//TODO: WIP angular vel
	angularVel += accumulatedTorque * invMass * deltaTime;

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
