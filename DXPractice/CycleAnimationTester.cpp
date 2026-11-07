#include "CycleAnimationTester.h"

#include "GameObject.h"
#include "Keyboard.h"
#include "SkeletalAnimatorComponent.h"
#include "TextRenderer.h"

void CycleAnimationTester::Start()
{
	animator = owner->GetComponent<SkeletalAnimatorComponent>();

	if (!animator)
	{
		SetEnabled(false);
		return;
	}

	clipNames = animator->GetClipNames();
	auto clipPlaying = std::ranges::find(clipNames, animator->GetCurrClipName());
	if (clipPlaying != clipNames.end())
	{
		currClipIndex = clipPlaying - clipNames.begin();
	}
	else
	{
		currClipIndex = 0;
	}
}

void CycleAnimationTester::Update(float deltaTime)
{
	if (!animator || clipNames.empty())
	{
		return;
	}

	if (keyboard->KeyIsTriggered('N'))
	{
		StopSequences();
		currClipIndex = (currClipIndex + 1) % clipNames.size();
		PlayCurrent();
	}
	if (keyboard->KeyIsTriggered('B'))
	{
		StopSequences();
		currClipIndex = (currClipIndex + clipNames.size() - 1) % clipNames.size();
		PlayCurrent();
	}
	if (keyboard->KeyIsTriggered('R'))
	{
		StopSequences();
		PlayCurrent();
	}
	if (keyboard->KeyIsTriggered('T'))
	{
		StopSequences();
		PlayByName("takeOff");
		chainTakeOffToFlyIdle = true;
	}
	if (keyboard->KeyIsTriggered('Y'))
	{
		// idle01 -> Walk (0.3s), attackHand lands halfway through that fade
		StartScript({ { 0.f, "idle01", 0.f },{ 1.f, "Walk", -1.f },{ 1.15f, "attackHand", -1.f } });
	}
	if (keyboard->KeyIsTriggered('M'))
	{
		slowMotion = !slowMotion;
		animator->SetTimeScale(slowMotion ? slowScale : 1.f);
	}

	if (chainTakeOffToFlyIdle && animator->IsFinished() && animator->GetCurrClipName() == "takeOff")
	{
		chainTakeOffToFlyIdle = false;
		PlayByName("FlyIdle");
	}
	UpdateScript(deltaTime * (slowMotion ? slowScale : 1.f));

	if (animator->IsFinished() && !finishReported)
	{
		OutputDebugStringA(("[AnimTest] " + animator->GetCurrClipName() + " finished\n").c_str());
		finishReported = true;
	}

}

void CycleAnimationTester::PlayCurrent()
{
	PlayByName(clipNames[currClipIndex]);
}

void CycleAnimationTester::PlayByName(const std::string& clipName, float fadeSec)
{
	if (!animator->PlayClip(clipName, fadeSec))
	{
		return;
	}
	finishReported = false;

	auto it = std::ranges::find(clipNames, clipName);
	if (it != clipNames.end())
	{
		currClipIndex = it - clipNames.begin();
	}

	char msg[128];
	sprintf_s(msg, "[AnimTest] %zu/%zu %s\n", currClipIndex + 1, clipNames.size(), clipName.c_str());
	OutputDebugStringA(msg);
}

void CycleAnimationTester::StartScript(std::vector<ScriptStep> steps)
{
	StopSequences();
	script = std::move(steps);
	UpdateScript(0.f);
}

void CycleAnimationTester::UpdateScript(float deltaTime)
{
	if (scriptIndex >= script.size())
	{
		return;
	}

	scriptTime += deltaTime;
	while (scriptIndex < script.size() && scriptTime >= script[scriptIndex].atSec)
	{
		const ScriptStep& step = script[scriptIndex++];
		PlayByName(step.clipName, step.fadeSec);
	}
}

void CycleAnimationTester::StopSequences()
{
	script.clear();
	scriptIndex = 0;
	scriptTime = 0.f;
	chainTakeOffToFlyIdle = false;
}
