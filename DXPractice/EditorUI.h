#pragma once
#include <string>

#include "AnimSetInspector.h"
#include "EditorViewport.h"
#include "HitVolumeInspector.h"

struct GameContext;
class GameObject;
class Scene;
class SceneManager;

class EditorUI
{
public:
	enum class Mode
	{
		EDIT,
		PLAY
	};

	void Draw(SceneManager& sceneManager, GameContext& context);
	void ToggleVisible();

	bool IsVisible() const
	{
		return isVisible;
	}

	bool IsPaused() const
	{
		return isPaused;
	}

	bool ShouldTick();

	Mode GetMode() const
	{
		return mode;
	}

	bool IsEditMode() const
	{
		return mode == Mode::EDIT;
	}

	bool WantsCursorLocked() const
	{
		return viewport.IsFlying();
	}

private:
	// UI widgets
	void DrawHierarchy(Scene& scene);
	void DrawInspector(Scene& scene);
	void DrawToolbar(SceneManager& sceneManager);
	void DrawLayoutSave(SceneManager& sceneManager);
	void DrawPrefabPalette(Scene& scene, GameContext& context);
	void DrawDebug(Scene& scene);

	void ValidateSelection(Scene& scene);

	void EnterPlayMode(SceneManager& sceneManager);
	void EnterEditMode(SceneManager& sceneManager);

	bool CanEditViewport() const
	{
		return mode == Mode::EDIT || isPaused;
	}

private:
	bool isVisible = false;
	bool isPaused = false;

	bool stepRequested = false;

	Scene* lastScene = nullptr;
	GameObject* selectedObject = nullptr;

	std::string saveMessage;
	bool saveValid = true;

	Mode mode = Mode::PLAY;
	std::string playModeStartScene;

	EditorViewport viewport;
	HitVolumeInspector hitVolumeInspector;
	AnimSetInspector animSetInspector;
};