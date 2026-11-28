#include "PlayerController.h"

#include "ActionController.h"
#include "ActionData.h"
#include "GameContext.h"
#include "GameObject.h"
#include "HealthComponent.h"
#include "HitVolumeComponent.h"
#include "Keyboard.h"
#include "LocomotionComponent.h"
#include "Mouse.h"
#include "PlayerStates.h"
#include "SkeletalAnimatorComponent.h"
#include "TextRenderer.h"

namespace
{
	constexpr float instantTurnRate = 80.f;
	constexpr auto comboWindowEvent = "ComboWindow";

	constexpr int maxTransitionsPerFrame = 4;
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
	actionController = owner->GetComponent<ActionController>();
	if (!locomotion || !animator || !actionController)
	{
#ifndef NDEBUG
		textRenderer->Log("[PlayerController] %s: needs LocomotionComponent and SkeletalAnimatorComponentand ActionController, controller disabled",
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

	RegisterStates();
	GetState(currStateID)->Enter(*this);
}

void PlayerController::Update(float deltaTime)
{
	const PlayerIntent intent = inputReader.ReadIntent(*keyboard, *mouse, *cam);

	if (isKnockdownPending)
	{
		isKnockdownPending = false;
		ForceState(PlayerStateID::KNOCKDOWN, true);
	}

	if (pendingStateID == noState)
	{
		GetState(currStateID)->Update(*this, intent, deltaTime);
	}

	ApplyPendingTransition(intent, deltaTime);

	ApplyRootMotion();
	locomotion->ApplyLocomotion(deltaTime);
}

void PlayerController::UpdateLocomotion(const PlayerIntent& intent)
{
	if (!intent.HasMove())
	{
		PlayLocomotionClip(idleClip);
		return;
	}

	const DirectX::XMFLOAT3& dir = intent.moveDir;
	locomotion->SetMove(dir, intent.isSprinting ? sprintSpeed : runSpeed);
	locomotion->SetFacing(atan2f(dir.x, dir.z), turnSpeed);
	PlayLocomotionClip(intent.isSprinting ? sprintClip : runClip);
}

void PlayerController::UpdateAction(const PlayerIntent& intent, float deltaTime)
{
	inputBuffer.Update(intent.pressedInput, deltaTime);
	const ActionButton bufferedInput = inputBuffer.Get();
	const DirectX::XMFLOAT3& dir = intent.moveDir;

	actionController->Tick();
	bool isOnChainFrame = actionController->DidCancelOpenThisFrame();
	bool isComboOpen = actionController->IsComboOpen();

	const bool isAttackBuffered = inputBuffer.HasAttack();
	const ActionDef* currAction = actionController->GetCurrAction();
	const bool hasFollowUp = currAction && (*currAction->nextOnAttack || *currAction->nextOnAttack2);
	const bool canMoveCancel = !isComboOpen || !hasFollowUp;

	if (actionController->IsCancelOpen())
	{
		if (bufferedInput == ActionButton::ROLL)
		{
			StartFromInput(bufferedInput, dir, !isOnChainFrame);
			return;
		}
		if (isAttackBuffered && isComboOpen)
		{
			StartFromInput(bufferedInput, dir, !isOnChainFrame);
			return;
		}

		if (canMoveCancel && bufferedInput == ActionButton::NONE && intent.HasMove())
		{
			RequestState(PlayerStateID::LOCOMOTION, true);
			return;
		}
	}

	if (actionController->IsFinished())
	{
		if (isAttackBuffered)
		{
			actionController->Stop();
			StartFromInput(bufferedInput, dir, false);
		}
		else
		{
			RequestState(PlayerStateID::LOCOMOTION);
		}
	}
}

void PlayerController::StartFromInput(ActionButton input, const DirectX::XMFLOAT3& dir, bool isLateChain)
{
	const ActionDef* currAction = actionController->GetCurrAction();
	const float minFade = isLateChain ? lateChainFadeSec : 0.f;

	switch (input)
	{
	case ActionButton::ROLL:
		StartAction(rollAction, dir, minFade);
		break;
	case ActionButton::ATTACK1:
		StartAction(currAction && *currAction->nextOnAttack ? currAction->nextOnAttack : attackAction,
		            dir, minFade);
		break;
	case ActionButton::ATTACK2:
		StartAction(currAction && *currAction->nextOnAttack2 ? currAction->nextOnAttack2 : attack2Action,
		            dir, minFade);
		break;
	case ActionButton::NONE:
		break;
	}
}

void PlayerController::StartAction(const char* actionName, const DirectX::XMFLOAT3& dir, float minFadeSec)
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

	actionController->Play(*action, minFadeSec);
	RequestState(PlayerStateID::ACTION);
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
	actionController->Stop();

	if (hitVolume)
	{
		hitVolume->SetAllHurtboxEnabled(false);
	}

	knockdownTimer = 0.f;
	knockdownGroundY = transform->GetPosition().y;
	isGettingUp = false;
	isGetUpCancelOpen = false;
}

void PlayerController::UpdateKnockdown(const PlayerIntent& intent, float deltaTime)
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

	if (!isGetUpCancelOpen && animator->WasMarkerFired(comboWindowEvent))
	{
		isGetUpCancelOpen = true;
		if (hitVolume)
		{
			hitVolume->SetAllHurtboxEnabled(true);
		}
	}

	if (isGetUpCancelOpen)
	{
		if (intent.pressedInput != ActionButton::NONE)
		{
			StartFromInput(intent.pressedInput, intent.moveDir, true);
			return;
		}
		if (intent.moveDir.x != 0.f || intent.moveDir.z != 0.f)
		{
			RequestState(PlayerStateID::LOCOMOTION, true);
			return;
		}
	}

	if (animator->IsFinished())
	{
		RequestState(PlayerStateID::LOCOMOTION);
	}
}

bool PlayerController::RequestState(PlayerStateID nextID, bool isRunNow)
{
	if (pendingStateID != noState)
	{
#ifndef NDEBUG
		textRenderer->Log("[PlayerController] %s ignored, %s already pending", ToString(nextID), ToString(pendingStateID));
#endif
		return false;
	}

	pendingStateID = nextID;
	isPendingRunNow = isRunNow;
	return true;
}

void PlayerController::ForceState(PlayerStateID nextID, bool isRunNow)
{
	pendingStateID = nextID;
	isPendingRunNow = isRunNow;
}

void PlayerController::RegisterStates()
{
	states[static_cast<size_t>(PlayerStateID::LOCOMOTION)] = std::make_unique<PlayerLocomotionState>();
	states[static_cast<size_t>(PlayerStateID::ACTION)] = std::make_unique<PlayerActionState>();
	states[static_cast<size_t>(PlayerStateID::KNOCKDOWN)] = std::make_unique<PlayerKnockdownState>();
}

PlayerState* PlayerController::GetState(PlayerStateID id) const
{
	if (id >= PlayerStateID::COUNT)
	{
		return nullptr;
	}

	return states[static_cast<size_t>(id)].get();
}

void PlayerController::ApplyPendingTransition(const PlayerIntent& intent, float deltaTime)
{
	for (int i = 0; i < maxTransitionsPerFrame && pendingStateID != noState; i++)
	{
		const PlayerStateID nextId = pendingStateID;
		const bool isRunNow = isPendingRunNow;
		pendingStateID = noState;
		isPendingRunNow = false;

		GetState(currStateID)->Exit(*this);
		RunTransitionCleanup(currStateID, nextId);
		currStateID = nextId;
		GetState(currStateID)->Enter(*this);

		if (isRunNow)
		{
			GetState(currStateID)->Update(*this, intent, deltaTime);
		}
	}

	assert(pendingStateID == noState && "player states keep requesting each other");
}

void PlayerController::RunTransitionCleanup(PlayerStateID fromID, PlayerStateID toID)
{
	if (fromID == PlayerStateID::KNOCKDOWN && hitVolume)
	{
		hitVolume->SetAllHurtboxEnabled(true);
	}

	if (fromID == PlayerStateID::KNOCKDOWN && toID != PlayerStateID::KNOCKDOWN)
	{
		DirectX::XMFLOAT3 pos = owner->GetTransform()->GetPosition();
		pos.y = knockdownGroundY;
		owner->GetTransform()->SetPosition(pos);
	}

	if (toID == PlayerStateID::LOCOMOTION)
	{
		actionController->Stop();
	}

	inputBuffer.Clear();
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
	const float scale = actionController->GetRootMotionScale();
	locomotion->AddMove({delta.x * scale, 0.f, delta.z * scale});
}
