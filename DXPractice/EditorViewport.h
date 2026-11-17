#pragma once
#include <DirectXMath.h>

class Camera2D;
class Camera3D;
class GameObject;
class Keyboard;
class Mouse;
class Scene;
struct GameContext;

// Scene-view interaction for the editor: click picking (2D sprites / 3D bounds / empty-object icons),
// dragging the selection, the right-click fly camera, and the selection / icon overlays.
// The selection itself is owned by EditorUI and passed in.
class EditorViewport
{
public:
	// left click selects (writes selectedObject), left drag moves it
	void HandleMouse(Scene& scene, GameContext& context, GameObject*& selectedObject);
	void HandleFlyCamera(Camera3D& cam, const Keyboard& keyboard, Mouse& mouse);

	void DrawSelectionOutline(GameObject& selectedObject, GameContext& context);
	void DrawObjectIcons(Scene& scene, GameContext& context);

	void CancelDrag()
	{
		isDragging = false;
	}

	void StopFlying()
	{
		isFlying = false;
	}

	bool IsFlying() const
	{
		return isFlying;
	}

private:
	bool BeginDrag3D(GameObject& object, GameContext& context, DirectX::XMFLOAT2 mouseScreenPos);
	void UpdateDrag3D(GameObject& object, GameContext& context, DirectX::XMFLOAT2 mouseScreenPos);

	GameObject* Pick(Scene& scene, GameContext& context, DirectX::XMFLOAT2 mousePosScreen);
	GameObject* Pick2D(Scene& scene, const Camera2D& camera, DirectX::XMFLOAT2 mousePosScreen);
	GameObject* Pick3D(Scene& scene, GameContext& context, DirectX::XMFLOAT2 mousePosScreen);

private:
	// drag
	bool isDragging = false;
	DirectX::XMFLOAT2 objDragOffset = {}; // 2d

	DirectX::XMFLOAT3 draggingPoint = {};
	DirectX::XMFLOAT3 dragPlaneNormal = {0.f, 1.f, 0.f};
	DirectX::XMFLOAT3 drag3DOffset = {};
	bool isDragVertical = false;

	// 3d cam fly mode
	bool isFlying = false;
	float flySpeed = 5.f;
	float lookSens = 0.004f;
};
