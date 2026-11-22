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
	constexpr auto invulnOffEvent = "InvulnOff";
	constexpr auto comboWindowEvent = "ComboWindow";
	constexpr auto comboWindowEndEvent = "ComboWindowEnd";
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
		health->onDamaged = [this](float, float, const HitEvent* event)
		{
			isKnockdownPending = true;
			hitDir = {};
			if (!event)
			{
				return;
			}

			const DirectX::XMFLOAT3 pos = owner->GetTransform()->GetPosition();
			DirectX::XMFLOAT3 source = event->contactPoint;
			float dx = source.x - pos.x;
			float dz = source.z - pos.z;

			if (dx * dx + dz * dz < 0.01f && event->attacker)
			{
				source = event->attacker->GetTransform()->GetPosition();
				dx = source.x - pos.x;
				dz = source.z - pos.z;
			}

			const float lengthSq = dx * dx + dz * dz;
			if (lengthSq > 1e-6f)
			{
				const float invLength = 1.f / sqrtf(lengthSq);
				hitDir = {dx * invLength, 0.f, dz * invLength};
			}
		};

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

	auto pressed = BufferedInput::NONE;
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

	if (isKnockdownPending)
	{
		isKnockdownPending = false;
		StartKnockdown();
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
	case PlayerState::KNOCKDOWN:
		UpdateKnockdown(dir, pressed, deltaTime);
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
	return {dirX * invLen, 0.f, dirZ * invLen};
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
	if (!isCancelOpen && animator->WasMarkerFired(comboWindowEvent))
	{
		isCancelOpen = true;
		isComboOpen = true;
		isOnChainFrame = true;
	}

	const bool isComboClosing = animator->WasMarkerFired(comboWindowEndEvent);

	const bool isAttackBuffered = bufferedInput == BufferedInput::ATTACK || bufferedInput == BufferedInput::ATTACK2;
	if (isCancelOpen)
	{
		if (bufferedInput == BufferedInput::ROLL)
		{
			StartFromInput(bufferedInput, dir, !isOnChainFrame);
			return;
		}
		if (isAttackBuffered && isComboOpen)
		{
			StartFromInput(bufferedInput, dir, !isOnChainFrame);
			return;
		}

		if (bufferedInput == BufferedInput::NONE && (dir.x != 0.f || dir.z != 0.f))
		{
			ChangeState(PlayerState::LOCOMOTION);
			UpdateLocomotion(dir);
			return;
		}
	}

	if (isComboClosing)
	{
		isComboOpen = false;
	}

	if (animator->IsFinished())
	{
		const BufferedInput pendingInput = isAttackBuffered ? bufferedInput : BufferedInput::NONE;
		ChangeState(PlayerState::LOCOMOTION);
		if (pendingInput != BufferedInput::NONE)
		{
			StartFromInput(pendingInput, dir, false);
		}
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

void PlayerController::StartKnockdown()
{
	TransformComponent* transform = owner->GetTransform();

	// face the hit
	if (hitDir.x != 0.f || hitDir.z != 0.f)
	{
		locomotion->SetFacing(atan2f(hitDir.x, hitDir.z), instantTurnRate);
		knockdownBackDir = {-hitDir.x, 0.f, -hitDir.z};
	}
	else
	{
		const float yaw = transform->GetRotation().y;
		knockdownBackDir = {-sinf(yaw), 0.f, -cosf(yaw)};
	}

	animator->PlayClip(knockdownClip, knockdownFadeSec, knockdownStartSec);
	ChangeState(PlayerState::KNOCKDOWN);
	currAction = nullptr;

	if (hitVolume)
	{
		hitVolume->SetAllHurtboxEnabled(false);
	}

	knockdownTimer = 0.f;
	knockdownGroundY = transform->GetPosition().y;
	isGettingUp = false;
}

void PlayerController::UpdateKnockdown(const DirectX::XMFLOAT3& dir, BufferedInput pressedInput, float deltaTime)
{
	TransformComponent* transform = owner->GetTransform();

	// procedural launch
	const float prevT = std::min(knockdownTimer / knockdownAirSec, 1.f);
	knockdownTimer += deltaTime;
	const float t = std::min(knockdownTimer / knockdownAirSec, 1.f);
	if (prevT < 1.f)
	{
		DirectX::XMFLOAT3 pos = transform->GetPosition();
		pos.y = knockdownGroundY + 4.f * knockdownAirHeight * t * (1.f - t);
		transform->SetPosition(pos);

		const auto pushAt = [this](float x)
			{
				return knockdownPushDistance * (1.f - (1.f - x) * (1.f - x));
			};
		const float step = pushAt(t) - pushAt(prevT);
		locomotion->AddMove({knockdownBackDir.x * step, 0.f, knockdownBackDir.z * step});
	}

	if (!isGettingUp)
	{
		if (animator->IsFinished())
		{
			animator->PlayClip(getUpClip, 0.f);
			isGettingUp = true;
		}
		return;
	}

	if (!isCancelOpen && animator->WasMarkerFired(comboWindowEvent))
	{
		isCancelOpen = true;
		if (hitVolume)
		{
			hitVolume->SetAllHurtboxEnabled(true);
		}
	}

	if (isCancelOpen)
	{
		if (pressedInput != BufferedInput::NONE)
		{
			StartFromInput(pressedInput, dir, true);
			return;
		}
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

void PlayerController::ChangeState(PlayerState nextState)
{
	// leaving a roll always ends its i-frames, whatever cut it short
	if ((state == PlayerState::ROLL || state == PlayerState::KNOCKDOWN) && hitVolume)
	{
		hitVolume->SetAllHurtboxEnabled(true);
	}

	if (state == PlayerState::KNOCKDOWN && nextState != PlayerState::KNOCKDOWN)
	{
		DirectX::XMFLOAT3 pos = owner->GetTransform()->GetPosition();
		pos.y = knockdownGroundY;
		owner->GetTransform()->SetPosition(pos);
	}

	if (nextState == PlayerState::LOCOMOTION)
	{
		currAction = nullptr;
	}

	bufferedInput = BufferedInput::NONE;
	isComboOpen = false;
	isCancelOpen = false;
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
	locomotion->AddMove({delta.x * scale, 0.f, delta.z * scale});
}
