#pragma once

class Camera3D;
class SceneManager;
class Camera2D;
class Renderer;
class PhysicsSystem;
class ScriptSystem;
class AnimationSystem;
class RenderSystem;
class Scene;
class Keyboard;
class Mouse;

struct GlobalContext
{
};

struct GameContext
{
	Renderer& renderer;
	PhysicsSystem& physicsSys;
	ScriptSystem& scriptSys;
	AnimationSystem& animationSys;
	RenderSystem& renderSys;
	Camera2D& cam2D;
	Camera3D& cam3D;

	Keyboard& keyboard;
	Mouse& mouse;

	Scene* gameScene = nullptr;
	SceneManager* sceneManager = nullptr;
	GlobalContext globalContext;
};
