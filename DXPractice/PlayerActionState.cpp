#include <cassert>

#include "ActionController.h"
#include "ActionData.h"
#include "PlayerController.h"
#include "PlayerStates.h"
#include "TextRenderer.h"

void PlayerActionState::Enter(PlayerController& player)
{
	inputBuffer.Clear();

	const ActionDef* action = player.GetRequestedAction();
	assert(action && "ACTION entered without StartAction");
	if (!action || !player.GetActionController().Play(*action, player.GetRequestedMinFade()))
	{
#ifndef NDEBUG
		player.textRenderer->Log("[PlayerController] action %s failed to play clip %s",
		                         action ? action->name : "(none)", action ? action->clip : "(none)");
#endif
		player.RequestState(PlayerStateID::LOCOMOTION);
	}
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
	const ActionDef* currAction = actionController.GetCurrAction();
	const bool hasFollowUp = currAction && (*currAction->nextOnAttack || *currAction->nextOnAttack2);
	const bool canMoveCancel = !isComboOpen || !hasFollowUp;

	if (actionController.IsCancelOpen())
	{
		if (bufferedInput == ActionButton::ROLL)
		{
			player.StartFromInput(bufferedInput, dir, !isOnChainFrame);
			return;
		}
		if (isAttackBuffered && isComboOpen)
		{
			player.StartFromInput(bufferedInput, dir, !isOnChainFrame);
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
			// StartFromInput picks the follow-up from the current action: stop first so this restarts at the opener
			actionController.Stop();
			player.StartFromInput(bufferedInput, dir, false);
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
}
