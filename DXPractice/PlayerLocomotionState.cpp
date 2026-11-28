#include "PlayerController.h"
#include "PlayerInput.h"
#include "PlayerStates.h"

void PlayerLocomotionState::Update(PlayerController& player, const PlayerIntent& intent, float deltaTime)
{
	if (intent.pressedInput != ActionButton::NONE)
	{
		player.StartFromInput(intent.pressedInput, intent.moveDir, false);
		return;
	}

	player.UpdateLocomotion(intent);
}
