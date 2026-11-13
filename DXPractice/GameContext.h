#pragma once

class HitDetectionSystem;
class TextRenderer;
class SkeletalAnimationSystem;
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
	TextRenderer& textRenderer;
	PhysicsSystem& physicsSys;
	HitDetectionSystem& hitDetectionSys;
	ScriptSystem& scriptSys;
	AnimationSystem& animationSys;
	SkeletalAnimationSystem& skeletalAnimationSys;
	RenderSystem& renderSys;
	Camera2D& cam2D;
	Camera3D& cam3D;

	Keyboard& keyboard;
	Mouse& mouse;

	Scene* currScene = nullptr;
	SceneManager* sceneManager = nullptr;
	GlobalContext globalContext;
};
