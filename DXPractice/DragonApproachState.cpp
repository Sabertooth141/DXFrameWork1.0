#include <DirectXCollision.h>

#include "DebugRenderer.h"
#include "DragonAI.h"
#include "DragonStates.h"
#include "LocomotionComponent.h"
#include "SkeletalAnimatorComponent.h"
#include "TransformComponent.h"


void DragonApproachState::Enter(DragonAI& ai)
{
	ai.GetAnimator().PlayClip(clipName);
}

void DragonApproachState::Update(DragonAI& ai, float deltaTime)
{
	const Perception& perception = ai.GetPerception();
	if (!perception.hasTarget)
	{
		ai.RequestStateTransition(StateID::IDLE);
		return;
	}

	// if has best attk - goto attack
	if (ai.RequestAttack(ai.GetAttackSelector().GetBestAttkIndex()))
	{
		return;
	}

	// if to0 far to the side; goto turn
	if (perception.sector != DirSector::FRONT)
	{
		ai.RequestStateTransition(StateID::TURN);
		return;
	}

	// close enough; goto idle
	if (perception.band == DistanceBand::CLOSE)
	{
		ai.RequestStateTransition(StateID::IDLE);
		return;
	}

	const float yaw = ai.GetTransform().GetRotation().y;
	LocomotionComponent& locomotion = ai.GetLocomotion();
	locomotion.SetMove({sinf(yaw), 0.f, cosf(yaw)}, GetWalkSpeed(ai));
	locomotion.SetFacing(yaw + DirectX::XMConvertToRadians(perception.angleDeg),
	                     DirectX::XMConvertToRadians(trackTurnRate));
}

#ifndef NDEBUG
void DragonApproachState::DrawDebug(const DragonAI& ai, DebugRenderer& debugRenderer) const
{
	// one second of travel along the walking direction: green
	const DirectX::XMFLOAT3 pos = ai.GetTransform().GetPosition();
	const float yaw = ai.GetTransform().GetRotation().y;
	const float length = GetWalkSpeed(ai);
	const DirectX::XMFLOAT3 from{ pos.x, pos.y + 0.1f, pos.z };
	const DirectX::XMFLOAT3 to{ from.x + sinf(yaw) * length, from.y, from.z + cosf(yaw) * length };
	debugRenderer.DrawLine3D(from, to, { 0.2f, 1.f, 0.3f, 1.f });
}
#endif

float DragonApproachState::GetWalkSpeed(const DragonAI& ai) const
{
	// so that the monster doesnt slide on ground
	return walkStrideSpeed * ai.GetTransform().GetScale().x * ai.GetAnimator().GetTimeScale();
}

