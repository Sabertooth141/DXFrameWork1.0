#include "SpriteAnimatorComponent.h"

#include "SpriteRendererComponent.h"

SpriteAnimatorComponent::SpriteAnimatorComponent(SpriteRendererComponent& sr, int cols, int rows, int totalFrames,
                                                 float fps) :
	srComp(sr),
	cols(cols), rows(rows),
	startFrame(0), endFrame(totalFrames - 1),
	currFrame(0),
	frameDuration(1.0f / fps),
	timer(0),
	loop(true)
{
	PushUV();
}

void SpriteAnimatorComponent::Update(float deltaTime)
{
	timer += deltaTime;
	if (timer >= frameDuration)
	{
		timer -= frameDuration;
		AdvanceFrame();
	}
}

void SpriteAnimatorComponent::SetAnimation(int inStartFrame, int inEndFrame, float fps, bool inLoop)
{
	if (startFrame == inStartFrame && endFrame == inEndFrame)
	{
		return;
	}
	startFrame = inStartFrame;
	endFrame = inEndFrame;
	currFrame = startFrame;
	frameDuration = 1.f / fps;
	timer = 0.f;
	loop = inLoop;
	PushUV();
}

void SpriteAnimatorComponent::AdvanceFrame()
{
	if (++currFrame > endFrame)
	{
		currFrame = loop ? startFrame : endFrame;
	}
	PushUV();
}

void SpriteAnimatorComponent::PushUV()
{
	float uvW = 1.f / static_cast<float>(cols);
	float uvH = 1.f / static_cast<float>(rows);

	srComp.UpdateUV({{ (currFrame % cols) * uvW, (currFrame / cols) * uvH }, { uvW, uvH }});
}
