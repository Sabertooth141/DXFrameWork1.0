#include "DebugRenderer.h"
#include "DragonStates.h"

#include "DragonAI.h"
#include "SkeletalAnimatorComponent.h"
#include "TransformComponent.h"

void DragonCalmState::Enter(DragonAI& ai)
{
	ai.GetAnimator().PlayClip(clipName);
}

void DragonCalmState::Update(DragonAI& ai, float deltaTime)
{
	const Perception& perception = ai.GetPerception();
	const bool isPlayerClose = perception.hasTarget && perception.distance < aggroRadius;

	if (isPlayerClose || ai.WasDamagedThisFrame())
	{
		ai.RequestStateTransition(StateID::TURN);
	}
}

#ifndef NDEBUG
void DragonCalmState::DrawDebug(const DragonAI& ai, DebugRenderer& debugRenderer) const
{
	const DirectX::XMFLOAT3 pos = ai.GetTransform().GetPosition();
	debugRenderer.DrawCircleXZ({ pos.x, pos.y + 0.05f, pos.z }, aggroRadius, { 1.f, 0.4f, 0.8f, 1.f });
}
#endif
