#pragma once
#include <string>
#include <vector>

#include "IComponent.h"

class SpriteRendererComponent;

struct SpriteFrame
{
	int w, h;
	int x, y;
	float duration;
};

class SpriteAnimatorComponent : public IComponent
{
public:
	SpriteAnimatorComponent(SpriteRendererComponent& sr, int cols, int rows, int totalFrames);
	SpriteAnimatorComponent(SpriteRendererComponent& sr, const std::wstring& jsonPath);
	void Update(float deltaTime) override;
	void SetAnimation(int inStartFrame, int inEndFrame, float fps, bool inLoop = true);
	bool ParseJson(const std::string& jsonString);
	SpriteRendererComponent* GetSpriteRenderer() const;

private:
	void AdvanceFrame();
	void PushUV();

	std::string ReadJson(const std::wstring& jsonPath);
	int ParseInt(const std::string& json, const std::string& key);

private:
	SpriteRendererComponent* srComp = nullptr;
	std::vector<SpriteFrame> animInfo;
	float spriteSheetW, spriteSheetH;
	int cols, rows;
	int startFrame, endFrame, currFrame;
	float frameDuration;
	float timer;
	bool loop;
};

