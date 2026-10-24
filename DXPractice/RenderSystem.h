#pragma once
#include "Camera3D.h"
#include "Camera2D.h"
#include "MeshRenderSystem.h"
#include "Renderer.h"
#include "SpriteRenderSystem.h"

class RenderSystem
{
public:
	RenderSystem(Renderer& renderer, Camera2D& cam2D, Camera3D& cam3D) : renderer(renderer), cam2D(cam2D), cam3D(cam3D)
	{
	}

	void Register(AnimatorComponent* sprite)
	{
		spriteSystem.Register(sprite);
	}

	void Register(MeshRendererComponent* meshRenderer)
	{
		meshSystem.Register(meshRenderer);
	}

	void Unregister(GameObject* object)
	{
		spriteSystem.Unregister(object);
		meshSystem.Unregister(object);
	}

	void Render()
	{
		Render3DPass();
		Render2DPass();
	}

private:
	void Render2DPass()
	{
		if (spriteSystem.IsEmpty())
		{
			return;
		}

		renderer.SetView(cam2D.GetView());
		renderer.Set2DMode();

		spriteSystem.Render();
	}

	void Render3DPass()
	{
		if (meshSystem.IsEmpty())
		{
			return;
		}

		renderer.SetView(cam3D.GetView());
		renderer.Set3DMode();

		meshSystem.Render(renderer);
	}

private:
	SpriteRenderSystem spriteSystem;
	MeshRenderSystem meshSystem;
	Renderer& renderer;

	Camera2D& cam2D;
	Camera3D& cam3D;
};
