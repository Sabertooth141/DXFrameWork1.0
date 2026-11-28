#include "PlayerController.h"
#include "PlayerStates.h"

void PlayerActionState::Update(PlayerController& player, const PlayerIntent& intent, float deltaTime)
{
	player.UpdateAction(intent, deltaTime);
}
