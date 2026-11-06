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
		currClipIndex = (currClipIndex + 1) % clipNames.size();
		PlayCurrent();
	}
	if (keyboard->KeyIsTriggered('B'))
	{
		currClipIndex = (currClipIndex + clipNames.size() - 1) % clipNames.size();
		PlayCurrent();
	}
	if (keyboard->KeyIsTriggered('R'))
	{
		PlayCurrent();
	}

	if (animator->IsFinished() && !finishReported)
	{
		OutputDebugStringA(("[AnimTest] " + animator->GetCurrClipName() + " finished\n").c_str());
		finishReported = true;
	}

}

void CycleAnimationTester::PlayCurrent()
{
	const std::string& clipName = clipNames[currClipIndex];
	animator->PlayClip(clipName);
	finishReported = false;

	char msg[128];
	sprintf_s(msg, "[AnimTest] %zu/%zu %s\n", currClipIndex + 1, clipNames.size(), clipName.c_str());
	OutputDebugStringA(msg);
}
