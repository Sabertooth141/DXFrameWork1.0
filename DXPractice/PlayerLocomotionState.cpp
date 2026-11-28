#include "LocomotionComponent.h"
#include "PlayerController.h"
#include "PlayerInput.h"
#include "PlayerStates.h"
#include "SkeletalAnimatorComponent.h"

void PlayerLocomotionState::Update(PlayerController& player, const PlayerIntent& intent, float deltaTime)
{
	if (intent.pressedInput != ActionButton::NONE)
	{
		player.RequestAction(intent.pressedInput, intent.moveDir, false);
		return;
	}

	if (!intent.HasMove())
	{
		PlayLocomotionClip(player, idleClip);
		return;
	}

	LocomotionComponent& locomotion = player.GetLocomotion();
	const DirectX::XMFLOAT3& dir = intent.moveDir;
	locomotion.SetMove(dir, intent.isSprinting ? sprintSpeed : runSpeed);
	locomotion.SetFacing(atan2f(dir.x, dir.z), turnSpeed);
	PlayLocomotionClip(player, intent.isSprinting ? sprintClip : runClip);
}

void PlayerLocomotionState::PlayLocomotionClip(PlayerController& player, const char* clipName) const
{
	SkeletalAnimatorComponent& animator = player.GetAnimator();
	if (animator.GetCurrClipName() != clipName)
	{
		animator.PlayClip(clipName, locomotionFadeSec);
	}
}
