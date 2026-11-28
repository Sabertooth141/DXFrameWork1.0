#include <algorithm>

#include "HitVolumeComponent.h"
#include "LocomotionComponent.h"
#include "PlayerController.h"
#include "PlayerStates.h"
#include "SkeletalAnimatorComponent.h"
#include "TransformComponent.h"

void PlayerKnockdownState::Enter(PlayerController& player)
{
	TransformComponent& transform = player.GetTransform();
	LocomotionComponent& locomotion = player.GetLocomotion();
	const DirectX::XMFLOAT3& hitDir = player.GetHitDir();

	// face the hit
	if (hitDir.x != 0.f || hitDir.z != 0.f)
	{
		locomotion.SetFacing(atan2f(hitDir.x, hitDir.z), instantTurnRate);
		knockdownBackDir = {-hitDir.x, 0.f, -hitDir.z};
	}
	else
	{
		const float yaw = transform.GetRotation().y;
		knockdownBackDir = {-sinf(yaw), 0.f, -cosf(yaw)};
	}

	player.GetAnimator().PlayClip(knockdownClip, knockdownFadeSec, knockdownStartSec);

	if (HitVolumeComponent* hitVolume = player.GetHitVolume())
	{
		hitVolume->SetAllHurtboxEnabled(false);
	}

	knockdownTimer = 0.f;
	knockdownGroundY = transform.GetPosition().y;
	isGettingUp = false;
	isGetUpCancelOpen = false;
}

void PlayerKnockdownState::Update(PlayerController& player, const PlayerIntent& intent, float deltaTime)
{
	TransformComponent& transform = player.GetTransform();
	SkeletalAnimatorComponent& animator = player.GetAnimator();

	// procedural launch
	const float prevT = std::min(knockdownTimer / knockdownAirSec, 1.f);
	knockdownTimer += deltaTime;
	const float t = std::min(knockdownTimer / knockdownAirSec, 1.f);
	if (prevT < 1.f)
	{
		DirectX::XMFLOAT3 pos = transform.GetPosition();
		pos.y = knockdownGroundY + 4.f * knockdownAirHeight * t * (1.f - t);
		transform.SetPosition(pos);

		const auto pushAt = [this](float x)
		{
			return knockdownPushDistance * (1.f - (1.f - x) * (1.f - x));
		};
		const float step = pushAt(t) - pushAt(prevT);
		player.GetLocomotion().AddMove({knockdownBackDir.x * step, 0.f, knockdownBackDir.z * step});
	}

	if (!isGettingUp)
	{
		if (animator.IsFinished())
		{
			animator.PlayClip(getUpClip, 0.f);
			isGettingUp = true;
		}
		return;
	}

	if (!isGetUpCancelOpen && animator.WasMarkerFired(comboWindowEvent))
	{
		isGetUpCancelOpen = true;
		if (HitVolumeComponent* hitVolume = player.GetHitVolume())
		{
			hitVolume->SetAllHurtboxEnabled(true);
		}
	}

	if (isGetUpCancelOpen)
	{
		if (intent.pressedInput != ActionButton::NONE)
		{
			player.RequestAction(intent.pressedInput, intent.moveDir, true);
			return;
		}
		if (intent.moveDir.x != 0.f || intent.moveDir.z != 0.f)
		{
			player.RequestState(PlayerStateID::LOCOMOTION, true);
			return;
		}
	}

	if (animator.IsFinished())
	{
		player.RequestState(PlayerStateID::LOCOMOTION);
	}
}

void PlayerKnockdownState::Exit(PlayerController& player)
{
	if (HitVolumeComponent* hitVolume = player.GetHitVolume())
	{
		hitVolume->SetAllHurtboxEnabled(true);
	}

	TransformComponent& transform = player.GetTransform();
	DirectX::XMFLOAT3 pos = transform.GetPosition();
	pos.y = knockdownGroundY;
	transform.SetPosition(pos);
}
