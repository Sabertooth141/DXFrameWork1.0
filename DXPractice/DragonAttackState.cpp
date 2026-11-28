#include <string>

#include "ActionController.h"
#include "ActionData.h"
#include "DebugRenderer.h"
#include "DragonAI.h"
#include "DragonStates.h"
#include "LocomotionComponent.h"
#include "SkeletalAnimatorComponent.h"
#include "TransformComponent.h"

void DragonAttackState::Enter(DragonAI& ai)
{
	isPlaying = false;
	isTracking = false;
	ai.ClearAttackHit();

	const int attkIndex = ai.GetCurrAttkIndex();
	DragonAttackSelector& selector = ai.GetAttackSelector();
	option = &selector.GetOption(attkIndex);

	selector.StartCooldown(attkIndex);

	const ActionDef* actionDef = FindAction(DragonActionData(), option->action);
	isPlaying = actionDef && ai.GetActionController().Play(*actionDef);
	isTracking = isPlaying && option->trackSpeedDeg > 0.f && ai.GetAnimator().HasEvent(actionDef->clip, trackEndEvent);
}

void DragonAttackState::Update(DragonAI& ai, float deltaTime)
{
	ActionController& actionController = ai.GetActionController();
	// TODO: for future combo implementation
	actionController.Tick();

	if (!isPlaying)
	{
		ai.RequestStateTransition(StateID::IDLE);
		return;;
	}

	if (isTracking && ai.GetAnimator().WasMarkerFired(trackEndEvent))
	{
		isTracking = false; // lock attk dir
	}

	const Perception& perception = ai.GetPerception();
	if (isTracking && perception.hasTarget)
	{
		const float yaw = ai.GetTransform().GetRotation().y;
		ai.GetLocomotion().SetFacing(yaw + DirectX::XMConvertToRadians(perception.angleDeg),
			DirectX::XMConvertToRadians(option->trackSpeedDeg));
	}
	
	if (actionController.IsFinished())
	{
		ai.RequestStateTransition(StateID::RECOVER);
	}
}

void DragonAttackState::Exit(DragonAI& ai)
{
	ai.GetActionController().Stop();
}

#ifndef NDEBUG
void DragonAttackState::DrawDebug(const DragonAI& ai, DebugRenderer& debugRenderer) const
{
	// facing line: yellow while tracking, red once locked
	constexpr float length = 8.f;
	const DirectX::XMFLOAT3 pos = ai.GetTransform().GetPosition();
	const float yaw = ai.GetTransform().GetRotation().y;
	const DirectX::XMFLOAT3 from{ pos.x, pos.y + 0.1f, pos.z };
	const DirectX::XMFLOAT3 to{ from.x + sinf(yaw) * length, from.y, from.z + cosf(yaw) * length };
	const DirectX::XMFLOAT4 color = isTracking ? DirectX::XMFLOAT4{ 1.f, 0.9f, 0.1f, 1.f } : DirectX::XMFLOAT4{ 1.f, 0.2f, 0.2f, 1.f };
	debugRenderer.DrawLine3D(from, to, color);
}
#endif	
