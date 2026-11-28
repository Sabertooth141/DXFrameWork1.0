#include <cassert>

#include "ActionController.h"
#include "ActionData.h"
#include "LocomotionComponent.h"
#include "PlayerController.h"
#include "PlayerMoveData.h"
#include "PlayerStates.h"
#include "SkeletalAnimatorComponent.h"
#include "TextRenderer.h"

void PlayerActionState::Enter(PlayerController& player)
{
	const ActionRequest& request = player.GetActionRequest();
	assert(request.button != ActionButton::NONE && "ACTION entered without RequestAction");
	StartMove(player, PickMove(request.button, nullptr), request.dir, request.isLateChain ? lateChainFadeSec : 0.f);
}

void PlayerActionState::Update(PlayerController& player, const PlayerIntent& intent, float deltaTime)
{
	ActionController& actionController = player.GetActionController();

	inputBuffer.Update(intent.pressedInput, deltaTime);
	const ActionButton bufferedInput = inputBuffer.Get();
	const DirectX::XMFLOAT3& dir = intent.moveDir;

	actionController.Tick();
	bool isOnChainFrame = actionController.DidCancelOpenThisFrame();
	bool isComboOpen = actionController.IsComboOpen();

	const bool isAttackBuffered = inputBuffer.HasAttack();
	const bool hasFollowUp = currMove && (*currMove->nextOnAttack || *currMove->nextOnAttack2);
	const bool canMoveCancel = !isComboOpen || !hasFollowUp;
	const float chainFadeSec = isOnChainFrame ? 0.f : lateChainFadeSec;

	if (actionController.IsCancelOpen())
	{
		if (bufferedInput == ActionButton::ROLL)
		{
			StartMove(player, PickMove(bufferedInput, currMove), dir, chainFadeSec);
			return;
		}
		if (isAttackBuffered && isComboOpen)
		{
			StartMove(player, PickMove(bufferedInput, currMove), dir, chainFadeSec);
			return;
		}

		if (canMoveCancel && bufferedInput == ActionButton::NONE && intent.HasMove())
		{
			player.RequestState(PlayerStateID::LOCOMOTION, true);
			return;
		}
	}

	if (actionController.IsFinished())
	{
		if (isAttackBuffered)
		{
			StartMove(player, PickMove(bufferedInput, nullptr), dir, 0.f);
		}
		else
		{
			player.RequestState(PlayerStateID::LOCOMOTION);
		}
	}
}

void PlayerActionState::Exit(PlayerController& player)
{
	player.GetActionController().Stop();
	currMove = nullptr;
}

#ifndef NDEBUG
void PlayerActionState::Validate(const PlayerController& player) const
{
	const std::vector<ActionDef>& actions = PlayerActionData();
	const std::vector<PlayerMove>& moves = PlayerMoveData();
	bool isValid = true;

	const auto checkMoveName = [&](const char* fromName, const char* moveName)
	{
		if (!FindMove(moves, moveName))
		{
			player.textRenderer->Log("[PlayerAction] %s: no move named %s", fromName, moveName);
			isValid = false;
		}
	};

	for (const ActionDef& action : actions)
	{
		if (!player.GetAnimator().HasClip(action.clip))
		{
			player.textRenderer->Log("[PlayerAction] action %s: clip %s is not loaded", action.name, action.clip);
			isValid = false;
		}
	}

	for (const PlayerMove& move : moves)
	{
		if (!FindAction(actions, move.action))
		{
			player.textRenderer->Log("[PlayerAction] move %s: no action named %s", move.name, move.action);
			isValid = false;
		}
		if (*move.nextOnAttack)
		{
			checkMoveName(move.name, move.nextOnAttack);
		}
		if (*move.nextOnAttack2)
		{
			checkMoveName(move.name, move.nextOnAttack2);
		}
	}

	checkMoveName("attackMove", attackMove);
	checkMoveName("attack2Move", attack2Move);
	checkMoveName("rollMove", rollMove);

	assert(isValid && "player action/move data has errors, see the on-screen log");
}
#endif

const char* PlayerActionState::PickMove(ActionButton button, const PlayerMove* fromMove) const
{
	switch (button)
	{
	case ActionButton::ROLL:
		return rollMove;
	case ActionButton::ATTACK1:
		if (fromMove && *fromMove->nextOnAttack)
		{
			return fromMove->nextOnAttack;
		}
		return attackMove;
	case ActionButton::ATTACK2:
		if (fromMove && *fromMove->nextOnAttack2)
		{
			return fromMove->nextOnAttack2;
		}

		return attack2Move;
	default:
		return nullptr;
	}
}

void PlayerActionState::StartMove(PlayerController& player, const char* moveName, const DirectX::XMFLOAT3& dir, float minFadeSec)
{
	ActionController& actionController = player.GetActionController();

	const PlayerMove* move;
	if (moveName)
	{
		move = FindMove(PlayerMoveData(), moveName);
	}
	else
	{
		move = nullptr;
	}

	const ActionDef* action;
	if (move)
	{
		action = FindAction(PlayerActionData(), move->action);
	}
	else
	{
		action = nullptr;
	}

	bool isPlaying = false;
	if (action)
	{
		if (dir.x != 0.f || dir.z != 0.f)
		{
			player.GetLocomotion().SetFacing(atan2f(dir.x, dir.z), instantTurnRate);
		}
		isPlaying = actionController.Play(*action, minFadeSec);
	}

	if (isPlaying)
	{
		inputBuffer.Clear();
		currMove = move;
		return;
	}

#ifndef NDEBUG
	player.textRenderer->Log("[PlayerAction] move %s failed (%s)", moveName ? moveName : "(none)",
	                         !move ? "no such move" : !action ? "no such action" : "clip failed to play");
#endif

	if (!actionController.IsActive())
	{
		currMove = nullptr;
		player.RequestState(PlayerStateID::LOCOMOTION);
	}
}
