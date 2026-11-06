#pragma once
#include <vector>

#include "MonoBehavior.h"

class SkeletalAnimatorComponent;

class CycleAnimationTester : public MonoBehavior
{
public:
	void Start() override;
	void Update(float deltaTime) override;

private:
	void PlayCurrent();

private:
	SkeletalAnimatorComponent* animator = nullptr;
	std::vector<std::string> clipNames;
	size_t currClipIndex = 0;
	bool finishReported = false;
};

