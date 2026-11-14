#include "CycleAnimationTester.h"

#include "GameObject.h"
#include "Keyboard.h"
#include "SkeletalAnimatorComponent.h"
#include "TextLabel.h"
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

	overlay = &owner->AddComponent<TextLabel>();
	overlay->isDrawingScreen = true;
	overlay->posOffset = {420.f, 120.f};

	for (const SkeletonNode& node : animator->GetSkeleton().nodes)
	{
		OutputDebugStringA(("[Bones]: " + node.name + "\n").c_str());
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
		StartScript({{0.f, "idle01", 0.f}, {1.f, "Walk", -1.f}, {1.15f, "attackHand", -1.f}});
	}
	if (keyboard->KeyIsTriggered('M'))
	{
		slowMotion = !slowMotion;
		ApplyTimeScale();
	}
	// event testing
	if (keyboard->KeyIsTriggered('U'))
	{
		StartScript({{0.f, "attackHand", 0.f}, {0.5f, "getHit", -1.f}});
	}
	if (keyboard->KeyIsTriggered('P'))
	{
		isPaused = !isPaused;
		ApplyTimeScale();
	}
	if (isPaused && keyboard->KeyIsTriggered(VK_RIGHT))
	{
		animator->Scrub(stepSec);
	}
	if (isPaused && keyboard->KeyIsTriggered(VK_LEFT))
	{
		animator->Scrub(-stepSec);
	}

	if (chainTakeOffToFlyIdle && animator->IsFinished() && animator->GetCurrClipName() == "takeOff")
	{
		chainTakeOffToFlyIdle = false;
		PlayByName("FlyIdle");
	}
	UpdateScript(deltaTime * CurrentTimeScale());

	if (animator->IsFinished() && !finishReported)
	{
		OutputDebugStringA(("[AnimTest] " + animator->GetCurrClipName() + " finished\n").c_str());
		finishReported = true;
	}
}

void CycleAnimationTester::LateUpdate(float deltaTime)
{
	if (!animator)
	{
		return;
	}

	HandleEvents();
	UpdateOverlay();
}

void CycleAnimationTester::OnHitDealt(const HitEvent& event)
{
	textRenderer->Log("[hit] %s #%u -> %s", event.hitboxGroup.c_str(), event.activation, event.hurtboxName.c_str());
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

float CycleAnimationTester::CurrentTimeScale() const
{
	if (isPaused)
	{
		return 0.f;
	}

	if (slowMotion)
	{
		return slowScale;
	}
	else
	{
		return 1.f;
	}
}

void CycleAnimationTester::ApplyTimeScale()
{
	animator->SetTimeScale(CurrentTimeScale());
}

void CycleAnimationTester::HandleEvents()
{
	for (const FiredAnimationEvent& event : animator->GetFiredEvents())
	{
		char line[160];
		if (event.type == FiredEventType::MARKER)
		{
			if (event.eventName == "HitboxOn")
			{
				hitboxClip = event.clipName;
			}
			else if (event.eventName == "HitboxOff")
			{
				hitboxClip.clear();
			}
			sprintf_s(line, "%s %s @%.3f (t=%.3f)", event.clipName.c_str(), event.eventName.c_str(),
			          event.timeSec, animator->GetCurrClipTime());
		}
		else // INTERRUPTED
		{
			const bool isCleanup = !hitboxClip.empty() && hitboxClip == event.clipName;
			if (isCleanup)
			{
				hitboxClip.clear();
			}
			sprintf_s(line, "%s INTERRUPTED @%.3f%s", event.clipName.c_str(), event.timeSec,
			          isCleanup ? " -> cleanup: hitbox forced off" : "");
		}

		textRenderer->Log("[animEvent] %s", line);
	}
}

void CycleAnimationTester::UpdateOverlay()
{
	char header[192];
	sprintf_s(header, "%s  %.3f / %.3f s%s%s%s\n",
		animator->GetCurrClipName().c_str(), animator->GetCurrClipTime(), animator->GetCurrClipLength(),
		isPaused ? "  [PAUSED  <- / -> 1/60s]" : "",
		slowMotion ? "  x0.2" : "",
		animator->IsFading() ? "  fading" : "");

	std::string text = header;
	if (!hitboxClip.empty())
	{
		text += "HITBOX ON (" + hitboxClip + ")\n";
	}
	for (const std::string& line : recentEvents)
	{
		text += line;
		text += '\n';
	}
	overlay->text = std::move(text);
}
