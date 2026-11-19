#include <algorithm>

#include "DebugRenderer.h"
#include "DragonAI.h"
#include "DragonPerception.h"
#include "DragonStates.h"
#include "LocomotionComponent.h"
#include "SkeletalAnimatorComponent.h"
#include "TransformComponent.h"

namespace
{
	constexpr float turn90LDeg = -90.f;
	constexpr float turn90RDeg = 90.f;
	constexpr float turn180Deg = 180.f; // turn right to 180

	auto clipName90L = "Turn90L";
	auto clipName90R = "Turn90R";
	auto clipName180 = "Turn180";

	float ToClipDirection(float predictedDeg, float currDeg, float clipDeg)
	{
		if (clipDeg == turn180Deg)
		{
			if (predictedDeg > 0.f)
			{
				return predictedDeg;
			}
			return predictedDeg + 360.f;
		}

		const bool isSameSide = (predictedDeg < 0.f) == (clipDeg < 0.f);
		if (isSameSide)
		{
			return predictedDeg;
		}
		else
		{
			return currDeg;
		}
	}
}

void DragonTurnState::Enter(DragonAI& ai)
{
	isTurning = false;

	const Perception& perception = ai.GetPerception();
	const float angleDeg = perception.angleDeg;
	const float absAngleDeg = fabsf(angleDeg);

	if (!perception.hasTarget || absAngleDeg < minTurnDeg)
	{
		return;
	}

	const char* clipName;
	float clipDeg;
	if (absAngleDeg <= turn180FromDeg)
	{
		const bool isLeft = angleDeg < 0.f;
		if (isLeft)
		{
			clipName = clipName90L;
			clipDeg = turn90LDeg;
		}
		else
		{
			clipName = clipName90R;
			clipDeg = turn90RDeg;
		}
	}
	else
	{
		clipName = clipName180;
		clipDeg = turn180Deg;
	}

	const float leadSec = ai.GetAnimator().GetClipLength(clipName) * leadScale;
	const float desiredDeg = ToClipDirection(PredictAngleDeg(ai, leadSec), angleDeg, clipDeg);

	finalWarpScale = std::clamp(desiredDeg / clipDeg, minWarpScale, maxWarpScale);
	targetYaw = ai.GetTransform().GetRotation().y + DirectX::XMConvertToRadians(clipDeg * finalWarpScale);
	isTurning = ai.GetAnimator().PlayClip(clipName);
}

void DragonTurnState::Update(DragonAI& ai, float deltaTime)
{
	if (isTurning)
	{
		ai.GetLocomotion().AddYaw(ai.GetAnimator().GetRootYawDelta() * finalWarpScale);

		if (!ai.GetAnimator().IsFinished())
		{
			return;
		}
	}

	ai.RequestStateTransition(GetNextState(ai));
}

#ifndef NDEBUG
void DragonTurnState::DrawDebug(const DragonAI& ai, DebugRenderer& debugRenderer) const
{
	if (!isTurning)
	{
		return;
	}

	// where the turn will end: orange line from the dragon
	constexpr float length = 8.f;
	const DirectX::XMFLOAT3 pos = ai.GetTransform().GetPosition();
	const DirectX::XMFLOAT3 from{pos.x, pos.y + 0.1f, pos.z};
	const DirectX::XMFLOAT3 to{from.x + sinf(targetYaw) * length, from.y, from.z + cosf(targetYaw) * length};
	debugRenderer.DrawLine3D(from, to, {1.f, 0.55f, 0.1f, 1.f});
}
#endif

StateID DragonTurnState::GetNextState(const DragonAI& ai) const
{
	return ai.IsInCombat() ? StateID::IDLE : StateID::ROAR;
}

float DragonTurnState::PredictAngleDeg(const DragonAI& ai, float leadSec) const
{
	const Perception& perception = ai.GetPerception();
	const DirectX::XMFLOAT3 pos = ai.GetTransform().GetPosition();
	const float yaw = ai.GetTransform().GetRotation().y;

	const float predictedX = perception.targetPos.x + perception.targetVel.x * leadSec;
	const float predictedZ = perception.targetPos.z + perception.targetVel.z * leadSec;
	const float toPredictedYaw = atan2f(predictedX - pos.x, predictedZ - pos.z);

	return DirectX::XMConvertToDegrees(remainderf(toPredictedYaw - yaw, DirectX::XM_2PI));
}

