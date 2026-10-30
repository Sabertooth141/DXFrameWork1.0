#pragma once
#include <DirectXMath.h>
#include <string>
class Mouse;
class Keyboard;
class Camera3D;
struct GameContext;
class PhysicsSystem;
class GameObject;
class Scene;
class Camera2D;
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
		return isFlying;
	}

private:
	// UI widgets
	void DrawHierarchy(Scene& scene);
	void DrawInspector(Scene& scene);
	void DrawToolbar(SceneManager& sceneManager);
	void HandleViewportMouse(Scene& scene, GameContext& context);
	void DrawSelectionOutline(GameContext& context);
	void DrawLayoutSave(SceneManager& sceneManager);
	void DrawPrefabPalette(Scene& scene, GameContext& context);
	void DrawPhysicsDebug(PhysicsSystem& physics);
	void DrawObjectIcon(Scene& scene, GameContext& context);

	// selection / drag
	void ValidateSelection(Scene& scene);
	bool BeginDrag3D(GameContext& context, DirectX::XMFLOAT2 mouseScreenPos);
	void UpdateDrag3D(GameContext& context, DirectX::XMFLOAT2 mouseScreenPos);

	// camera
	void HandleEditorCamera(Camera3D& cam, const Keyboard& keyboard, Mouse& mouse);

	GameObject* Pick(Scene& scene, GameContext& context, DirectX::XMFLOAT2 mousePosScreen);
	GameObject* Pick2D(Scene& scene, const Camera2D& camera, DirectX::XMFLOAT2 mousePosScreen);
	GameObject* Pick3D(Scene& scene, GameContext& context, DirectX::XMFLOAT2 mousePosScreen);

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

	// drag
	bool isDragging = false;
	DirectX::XMFLOAT2 objDragOffset = {}; // 2d

	DirectX::XMFLOAT3 draggingPoint = {};
	DirectX::XMFLOAT3 dragPlaneNormal = {0.f, 1.f, 0.f};
	DirectX::XMFLOAT3 drag3DOffset = {};
	bool isDragVertical = false;

	std::string saveMessage;
	bool saveValid = true;

	Mode mode = Mode::PLAY;
	std::string playModeStartScene;

	// 3d cam fly mode
	bool isFlying = false;
	float flySpeed = 5.f;
	float lookSens = 0.004f;
};
