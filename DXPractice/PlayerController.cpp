#include "PlayerController.h"

#include "GameObject.h"
#include "Keyboard.h"
#include "WindowSettings.h"

void PlayerController::Start()
{
	MonoBehavior::Start();
	posX = owner->GetTransform()->GetPosition().x;
	posY = owner->GetTransform()->GetPosition().y;
}

void PlayerController::Update(float deltaTime)
{
	MonoBehavior::Update(deltaTime);
	HandleInput(deltaTime);
	HandleAnimation(deltaTime);
	HandleMovement(deltaTime);
}

void PlayerController::Awake()
{
	MonoBehavior::Awake();
}

void PlayerController::OnDestroy()
{
	MonoBehavior::OnDestroy();
}

void PlayerController::HandleInput(float deltaTime)
{
	if (keyboard->KeyIsPressed('D'))
	{
		owner->GetComponent<AnimatorComponent>()->SetFlipX(false);
		currSpeedX = std::min(currSpeedX + accX * deltaTime, maxSpeedX);
	}

	if (keyboard->KeyIsPressed('A'))
	{
		owner->GetComponent<AnimatorComponent>()->SetFlipX(true);
		currSpeedX = std::max(currSpeedX - accX * deltaTime, -maxSpeedX);
	}
}

void PlayerController::HandleAnimation(float deltaTime)
{
	if (currSpeedX == 0 && owner->GetComponent<AnimatorComponent>()->GetCurrAnimName() != "CharIdle")
	{
		owner->GetComponent<AnimatorComponent>()->SetCurrAnimation("CharIdle");
	}
	else if (currSpeedX != 0 && owner->GetComponent<AnimatorComponent>()->GetCurrAnimName() != "CharMove")
	{
		owner->GetComponent<AnimatorComponent>()->SetCurrAnimation("CharMove");
	}
}

void PlayerController::HandleMovement(float deltaTime)
{

	float halfSprite = owner->GetTransform()->GetScale().x / 2.0f;
	float halfW = WIN_WIDTH / 2.0f;

	if (currSpeedX > 0)
	{
		currSpeedX = std::max(0.f, currSpeedX - frictionX * deltaTime);
	}
	else if (currSpeedX < 0)
	{
		currSpeedX = std::min(0.f, currSpeedX + frictionX * deltaTime);
	}
	posX += currSpeedX;
	if (posX + halfSprite < -WIN_WIDTH / 2)
	{
		posX = halfSprite + WIN_WIDTH / 2;
	}

	if (posX - halfSprite > WIN_WIDTH / 2)
	{
		posX = -halfSprite - WIN_WIDTH / 2;
	}
	owner->GetTransform()->SetPosition({ posX, posY, 1 });
}
