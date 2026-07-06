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

	if (keyboard->KeyIsPressed('W'))
	{
		currSpeedY = std::min(currSpeedY + accY * deltaTime, maxSpeedY);
	}

	if (keyboard->KeyIsPressed('S'))
	{
		currSpeedY = std::max(currSpeedY - accY * deltaTime, -maxSpeedY);
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
	float halfSpriteX = owner->GetTransform()->GetScale().x / 2.0f;
	float halfSpriteY = owner->GetTransform()->GetScale().y / 2.0f;

	// X friction
	if (currSpeedX > 0)
	{
		currSpeedX = std::max(0.f, currSpeedX - frictionX * deltaTime);
	}
	else if (currSpeedX < 0)
	{
		currSpeedX = std::min(0.f, currSpeedX + frictionX * deltaTime);
	}

	// Y friction
	if (currSpeedY > 0)
	{
		currSpeedY = std::max(0.f, currSpeedY - frictionY * deltaTime);
	}
	else if (currSpeedY < 0)
	{
		currSpeedY = std::min(0.f, currSpeedY + frictionY * deltaTime);
	}

	posX += currSpeedX;
	posY += currSpeedY;

	// X wrap
	if (posX + halfSpriteX < -WIN_WIDTH / 2)
	{
		posX = halfSpriteX + WIN_WIDTH / 2;
	}
	if (posX - halfSpriteX > WIN_WIDTH / 2)
	{
		posX = -halfSpriteX - WIN_WIDTH / 2;
	}

	// Y wrap
	if (posY + halfSpriteY < -WIN_HEIGHT / 2)
	{
		posY = halfSpriteY + WIN_HEIGHT / 2;
	}
	if (posY - halfSpriteY > WIN_HEIGHT / 2)
	{
		posY = -halfSpriteY - WIN_HEIGHT / 2;
	}

	owner->GetTransform()->SetPosition({ posX, posY, 1 });
}

void PlayerController::OnCollisionEnter2D(const GameObject& other)
{
	MonoBehavior::OnCollisionEnter2D(other);
}
