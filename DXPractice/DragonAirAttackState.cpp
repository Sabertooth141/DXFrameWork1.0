#include "DebugRenderer.h"
#include "DragonAI.h"
#include "DragonStates.h"
#include "LocomotionComponent.h"
#include "SkeletalAnimatorComponent.h"
#include "TransformComponent.h"

namespace
{
	const std::string strafeStartEvent = "StrafeStart";
	const std::string strafeEndEvent = "StrafeEnd";
}

void DragonAirAttackState::Enter(DragonAI& ai)
{
	SkeletalAnimatorComponent& animator = ai.GetAnimator();
	isPlaying = animator.PlayClip(clipName);
	isTracking = isPlaying && trackSpeedDeg > 0.f && animator.HasEvent(clipName, trackEndEvent);
	ai.GetStrafeMotion().Reset();
}

void DragonAirAttackState::Update(DragonAI& ai, float deltaTime)
{
	SkeletalAnimatorComponent& animator = ai.GetAnimator();

	if (isTracking && animator.WasMarkerFired(trackEndEvent))
	{
		isTracking = false;
	}

	StrafeMotion& strafe = ai.GetStrafeMotion();
	if (animator.WasMarkerFired(strafeStartEvent))
	{
		strafe.Begin(strafeSpeed);
	}
	if (animator.WasMarkerFired(strafeEndEvent))
	{
		strafe.BeginSlowdown();
	}

	const float currStrafeSpeed = strafe.Update(deltaTime);
	if (currStrafeSpeed > 0.f)
	{
		const float yaw = ai.GetTransform().GetRotation().y;
		ai.GetLocomotion().SetMove({sinf(yaw), 0.f, cosf(yaw)}, currStrafeSpeed);
	}

	const Perception& perception = ai.GetPerception();
	if (isTracking && perception.hasTarget)
	{
		const float yaw = ai.GetTransform().GetRotation().y;
		ai.GetLocomotion().SetFacing(yaw + DirectX::XMConvertToRadians(perception.angleDeg),
		                             DirectX::XMConvertToRadians(trackSpeedDeg));
	}

	if (!isPlaying || animator.IsFinished())
	{
		ai.RequestStateTransition(StateID::AIR_RECOVER);
	}
}

#ifndef NDEBUG
void DragonAirAttackState::DrawDebug(const DragonAI& ai, DebugRenderer& debugRenderer) const
{
	// facing line: yellow while tracking, red once locked (same as the ground Attack)
	constexpr float length = 10.f;
	const DirectX::XMFLOAT3 pos = ai.GetTransform().GetPosition();
	const float yaw = ai.GetTransform().GetRotation().y;
	const DirectX::XMFLOAT3 from{pos.x, pos.y + 0.1f, pos.z};
	const DirectX::XMFLOAT3 to{from.x + sinf(yaw) * length, from.y, from.z + cosf(yaw) * length};
	const DirectX::XMFLOAT4 color = isTracking ? DirectX::XMFLOAT4{1.f, 0.9f, 0.1f, 1.f} : DirectX::XMFLOAT4{1.f, 0.2f, 0.2f, 1.f};
	debugRenderer.DrawLine3D(from, to, color);
}
#endif
