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

#ifndef NDEBUG
	ValidateActionData();
#endif

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

TransformComponent& PlayerController::GetTransform() const
{
	return *owner->GetTransform();
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

	// played in PlayerActionState::Enter, not here: ACTION's Exit stops during the transition and would kill it
	requestedAction = action;
	requestedMinFadeSec = minFadeSec;
	RequestState(PlayerStateID::ACTION);
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
		currStateID = nextId;
		GetState(currStateID)->Enter(*this);

		if (isRunNow)
		{
			GetState(currStateID)->Update(*this, intent, deltaTime);
		}
	}

	assert(pendingStateID == noState && "player states keep requesting each other");
}

void PlayerController::ApplyRootMotion()
{
	const DirectX::XMFLOAT3& delta = animator->GetRootMoveDelta();
	const float scale = actionController->GetRootMotionScale();
	locomotion->AddMove({delta.x * scale, 0.f, delta.z * scale});
}

#ifndef NDEBUG
void PlayerController::ValidateActionData() const
{
	const std::vector<ActionDef>& actions = PlayerActionData();
	bool isValid = true;

	const auto checkActionName = [&](const char* fromName, const char* actionName)
	{
		if (!FindAction(actions, actionName))
		{
			textRenderer->Log("[PlayerController] %s: no action named %s", fromName, actionName);
			isValid = false;
		}
	};

	for (const ActionDef& action : actions)
	{
		if (!animator->HasClip(action.clip))
		{
			textRenderer->Log("[PlayerController] action %s: clip %s is not loaded", action.name, action.clip);
			isValid = false;
		}

		if (*action.nextOnAttack)
		{
			checkActionName(action.name, action.nextOnAttack);
		}
		if (*action.nextOnAttack2)
		{
			checkActionName(action.name, action.nextOnAttack2);
		}
	}

	checkActionName("attackAction", attackAction);
	checkActionName("attack2Action", attack2Action);
	checkActionName("rollAction", rollAction);

	assert(isValid && "player action data has errors, see the on-screen log");
}
#endif
