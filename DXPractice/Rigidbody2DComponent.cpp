#include "Rigidbody2DComponent.h"

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
	velocity.y += accumulatedForce.y * invMass * deltaTime;

	// lin dampening
	velocity.x *= (1.0f - linearDampening);
	velocity.y *= (1.0f - linearDampening);

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
