#pragma once
#include "IComponent.h"

class SpriteRendererComponent;

class SpriteAnimatorComponent : public IComponent
{
public:
	SpriteAnimatorComponent(SpriteRendererComponent& sr, int cols, int rows, int totalFrames, float fps);
	void Update(float deltaTime) override;
	void SetAnimation(int inStartFrame, int inEndFrame, float fps, bool inLoop = true);

private:
	void AdvanceFrame();
	void PushUV();

	SpriteRendererComponent& srComp;
	int cols, rows;
	int startFrame, endFrame, currFrame;
	float frameDuration;
	float timer;
	bool loop;
};

