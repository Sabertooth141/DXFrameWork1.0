#pragma once
#include "IComponent.h"
#include "MonoBehavior.h"

class PlayerController : MonoBehavior
{
public:
	void Start() override;
	void Update(float deltaTime) override;
	~PlayerController() override;
};

