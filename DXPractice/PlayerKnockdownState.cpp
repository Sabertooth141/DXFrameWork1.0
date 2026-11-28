#include "PlayerController.h"
#include "PlayerStates.h"

void PlayerKnockdownState::Enter(PlayerController& player)
{
	player.StartKnockdown();
}

void PlayerKnockdownState::Update(PlayerController& player, const PlayerIntent& intent, float deltaTime)
{
	player.UpdateKnockdown(intent, deltaTime);
}
