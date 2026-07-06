#pragma once
#include "IComponent.h"
#include "MonoBehavior.h"

class PlayerController : public MonoBehavior
{
public:
	void Start() override;
	void Update(float deltaTime) override;
	~PlayerController() override = default;
	void Awake() override;
	void OnDestroy() override;

	void HandleInput(float deltaTime);
	void HandleAnimation(float deltaTime);
	void HandleMovement(float deltaTime);

	void OnCollisionEnter2D(const GameObject& other) override;

private:
	float accX = 10;
	float currSpeedX = 0;
	float maxSpeedX = 100;
	float posX = 0, posY = 0;
	float frictionX = 8;
	float currSpeedY = 0.0f;
	float accY = 10;
	float maxSpeedY = 100;
	float frictionY = 8;
};

