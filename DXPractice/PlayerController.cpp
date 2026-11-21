#include "PlayerController.h"

#include "ActionData.h"
#include "GameContext.h"
#include "GameObject.h"
#include "HealthComponent.h"
#include "HitVolumeComponent.h"
#include "Keyboard.h"
#include "LocomotionComponent.h"
#include "Mouse.h"
#include "SkeletalAnimatorComponent.h"
#include "TextRenderer.h"

namespace
{
	constexpr float instantTurnRate = 1000.f;
	constexpr auto invulnOnEvent = "InvulnOn";
	constexpr auto invulnOffEvent = "InVulnOff";
	constexpr auto comboWindowEvent = "ComboWindow";
}

void PlayerController::Awake()
{
	MonoBehavior::Awake();

	cam = &owner->GetContext().cam3D;
}

void PlayerController::Start()
{
	MonoBehavior::Start();

	locomotion = owner->GetComponent<LocomotionComponent>();
	animator = owner->GetComponent<SkeletalAnimatorComponent>();
	if (!locomotion || !animator)
	{
#ifndef NDEBUG
		textRenderer->Log("[PlayerController] %s: needs LocomotionComponent and SkeletalAnimatorComponent, controller disabled",
		                  owner->GetName().c_str());
#endif
		SetEnabled(false);
		return;
	}

	hitVolume = owner->GetComponent<HitVolumeComponent>();

	health = owner->GetComponent<HealthComponent>();
	if (health)
	{
		health->onDeath = [this]()
		{
			health->Reset();
		};
	}
}

void PlayerController::Update(float deltaTime)
{
	// TODO: temp
	const DirectX::XMFLOAT3 dir = ReadMoveInput();

	BufferedInput pressed = BufferedInput::NONE;
	if (ReadAttackTriggered())
	{
		pressed = BufferedInput::ATTACK;
	}
	if (keyboard->KeyIsTriggered('F'))
	{
		pressed = BufferedInput::ATTACK2;
	}
	if (keyboard->KeyIsTriggered(VK_SPACE))
	{
		pressed = BufferedInput::ROLL; // wins a same-frame tie
	}

	switch (state)
	{
	case PlayerState::LOCOMOTION:
		if (pressed != BufferedInput::NONE)
		{
			StartFromInput(pressed, dir, false);
		}
		else
		{
			UpdateLocomotion(dir);
		}
		break;
	case PlayerState::ATTACK:
	case PlayerState::ROLL:
		UpdateAction(dir, pressed, deltaTime);
		break;
	case PlayerState::HIT:
		break;
	}

	ApplyRootMotion();
	locomotion->ApplyLocomotion(deltaTime);
}

DirectX::XMFLOAT3 PlayerController::ReadMoveInput() const
{
	float inputX = 0.f;
	float inputZ = 0.f;
	if (keyboard->KeyIsPressed('W'))
	{
		inputZ += 1.f;
	}
	if (keyboard->KeyIsPressed('S'))
	{
		inputZ -= 1.f;
	}
	if (keyboard->KeyIsPressed('D'))
	{
		inputX += 1.f;
	}
	if (keyboard->KeyIsPressed('A'))
	{
		inputX -= 1.f;
	}

	if (inputX == 0.f && inputZ == 0.f)
	{
		return {};
	}

	// camera-relative: W is "away from the camera", whatever way it faces
	const float sinYaw = sinf(cam->yaw);
	const float cosYaw = cosf(cam->yaw);
	const float dirX = sinYaw * inputZ + cosYaw * inputX;
	const float dirZ = cosYaw * inputZ - sinYaw * inputX;

	// normalize so diagonals aren't faster
	const float invLen = 1.f / sqrtf(dirX * dirX + dirZ * dirZ);
	return { dirX * invLen, 0.f, dirZ * invLen };
}

bool PlayerController::ReadAttackTriggered()
{
	const bool isLeftDown = mouse->LeftPressed();
	const bool istriggered = isLeftDown && !wasLeftDown;
	wasLeftDown = isLeftDown;
	return istriggered;
}

void PlayerController::UpdateLocomotion(const DirectX::XMFLOAT3& dir)
{
	if (dir.x == 0.f && dir.z == 0.f)
	{
		PlayLocomotionClip(idleClip);
		return;
	}

	const bool isSprinting = keyboard->KeyIsPressed(VK_SHIFT);
	locomotion->SetMove(dir, isSprinting ? sprintSpeed : runSpeed);
	locomotion->SetFacing(atan2f(dir.x, dir.z), turnSpeed);
	PlayLocomotionClip(isSprinting ? sprintClip : runClip);
}

void PlayerController::UpdateAction(const DirectX::XMFLOAT3& dir, BufferedInput pressedInput, float deltaTime)
{
	if (state == PlayerState::ROLL)
	{
		UpdateInvuln();
	}

	if (pressedInput != BufferedInput::NONE)
	{
		bufferedInput = pressedInput;
		bufferAgeSec = 0.f;
	}
	else if (bufferedInput != BufferedInput::NONE)
	{
		bufferAgeSec += deltaTime;
		if (bufferAgeSec > bufferSec)
		{
			bufferedInput = BufferedInput::NONE;
		}
	}

	bool isOnChainFrame = false;
	if (!isWindowOpen && animator->WasMarkerFired(comboWindowEvent))
	{
		isWindowOpen = true;
		isOnChainFrame = true;
	}

	if (isWindowOpen)
	{
		if (bufferedInput != BufferedInput::NONE)
		{
			StartFromInput(bufferedInput, dir, !isOnChainFrame);
			return;
		}

		// nothing queued: moving cuts the recovery
		if (dir.x != 0.f || dir.z != 0.f)
		{
			ChangeState(PlayerState::LOCOMOTION);
			UpdateLocomotion(dir);
			return;
		}
	}

	if (animator->IsFinished())
	{
		ChangeState(PlayerState::LOCOMOTION);
	}
}

void PlayerController::UpdateInvuln()
{
	if (!hitVolume)
	{
		return;
	}

	if (animator->WasMarkerFired(invulnOnEvent))
	{
		hitVolume->SetAllHurtboxEnabled(false);
	}
	if (animator->WasMarkerFired(invulnOffEvent))
	{
		hitVolume->SetAllHurtboxEnabled(true);
	}
}

void PlayerController::StartFromInput(BufferedInput input, const DirectX::XMFLOAT3& dir, bool isLateChain)
{
	const float minFade = isLateChain ? lateChainFadeSec : 0.f;

	switch (input)
	{
	case BufferedInput::ROLL:
		StartAction(rollAction, dir, PlayerState::ROLL, minFade);
		break;
	case BufferedInput::ATTACK:
		StartAction(currAction && *currAction->nextOnAttack ? currAction->nextOnAttack : attackAction,
			dir, PlayerState::ATTACK, minFade);
		break;
	case BufferedInput::ATTACK2:
		StartAction(currAction && *currAction->nextOnAttack2 ? currAction->nextOnAttack2 : attack2Action,
			dir, PlayerState::ATTACK, minFade);
		break;
	case BufferedInput::NONE:
		break;
	}
}

void PlayerController::StartAction(const char* actionName, const DirectX::XMFLOAT3& dir, PlayerState nextState, float minFadeSec)
{
	const ActionDef* action = FindAction(PlayerActionData(), actionName);
	if (!action)
	{
#ifndef NDEBUG
		textRenderer->Log("[PlayerController] no action named %s", actionName);
#endif
		return;
	}

	if (dir.x != 0.f || dir.z != 0.f)
	{
		locomotion->SetFacing(atan2f(dir.x, dir.z), instantTurnRate);
	}

	animator->PlayClip(action->clip, std::max(action->fadeInSec, minFadeSec));
	ChangeState(nextState);
	currAction = action;
}

void PlayerController::ChangeState(PlayerState nextState)
{
	// leaving a roll always ends its i-frames, whatever cut it short
	if (state == PlayerState::ROLL && hitVolume)
	{
		hitVolume->SetAllHurtboxEnabled(true);
	}

	if (nextState == PlayerState::LOCOMOTION)
	{
		currAction = nullptr;
	}

	bufferedInput = BufferedInput::NONE;
	isWindowOpen = false;
	state = nextState;
}

void PlayerController::PlayLocomotionClip(const char* clipName)
{
	if (animator->GetCurrClipName() != clipName)
	{
		animator->PlayClip(clipName, locomotionFadeSec);
	}
}

void PlayerController::ApplyRootMotion()
{
	const DirectX::XMFLOAT3& delta = animator->GetRootMoveDelta();
	const float scale = currAction ? currAction->rootMotionScale : 1.f;
	locomotion->AddMove({ delta.x * scale, 0.f, delta.z * scale });
}

