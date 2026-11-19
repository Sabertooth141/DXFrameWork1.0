#include "DebugRenderer.h"
#include "DragonAI.h"
#include "DragonStates.h"
#include "LocomotionComponent.h"
#include "SkeletalAnimatorComponent.h"
#include "TransformComponent.h"

void DragonAirAimState::Enter(DragonAI& ai)
{
	ai.GetAnimator().PlayClip(clipName);
}

void DragonAirAimState::Update(DragonAI& ai, float deltaTime)
{
	const Perception& perception = ai.GetPerception();
	const float timeInState = ai.GetTimeInState();

	if (perception.hasTarget)
	{
		LocomotionComponent& locomotion = ai.GetLocomotion();
		const DirectX::XMFLOAT3 pos = ai.GetTransform().GetPosition();
		const float yaw = ai.GetTransform().GetRotation().y;

		locomotion.SetFacing(yaw + DirectX::XMConvertToRadians(perception.angleDeg),
			DirectX::XMConvertToRadians(turnRate));

		const bool isFarEnough = perception.distance >= retreatDistance;
		if (!isFarEnough && perception.distance > 1e-3f)
		{
			const float awayX = (pos.x - perception.targetPos.x) / perception.distance;
			const float awayZ = (pos.z - perception.targetPos.z) / perception.distance;
			locomotion.SetMove({ awayX, 0.f, awayZ }, retreatSpeed);
		}

		const bool isAimed = fabsf(perception.angleDeg) <= aimToleranceDeg;
		if (timeInState >= minAimSec && isFarEnough && isAimed)
		{
			ai.RequestStateTransition(StateID::AIR_ATTACK);
			return;
		}
	}

	if (timeInState >= maxAimSec)
	{
		ai.RequestStateTransition(StateID::AIR_ATTACK);
	}
}

#ifndef NDEBUG
void DragonAirAimState::DrawDebug(const DragonAI& ai, DebugRenderer& debugRenderer) const
{
	// cyan ring: the retreat distance, the player is "far enough" outside it
	const DirectX::XMFLOAT3 pos = ai.GetTransform().GetPosition();
	debugRenderer.DrawCircleXZ({ pos.x, pos.y + 0.1f, pos.z }, retreatDistance, { 0.2f, 0.9f, 1.f, 1.f });
}
#endif
