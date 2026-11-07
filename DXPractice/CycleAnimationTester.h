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
	struct ScriptStep
	{
		float atSec;
		const char* clipName;
		float fadeSec; // < 0 = fade table
	};

	void PlayCurrent();
	void PlayByName(const std::string& clipName, float fadeSec = -1.f);
	void StartScript(std::vector<ScriptStep> steps);
	void UpdateScript(float deltaTime);
	void StopSequences();

private:
	static constexpr float slowScale = 0.2f;

	SkeletalAnimatorComponent* animator = nullptr;
	std::vector<std::string> clipNames;
	size_t currClipIndex = 0;
	bool finishReported = false;

	// cross fade testing
	std::vector<ScriptStep> script;
	size_t scriptIndex = 0;
	float scriptTime = 0.f;
	bool chainTakeOffToFlyIdle = false;
	bool slowMotion = false;
};

