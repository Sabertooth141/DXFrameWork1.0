#pragma once
#include <set>

#include "Camera3D.h"
#include "Camera2D.h"
#include "LightCBuffer.h"
#include "LightComponent.h"
#include "MeshRenderSystem.h"
#include "Renderer.h"
#include "SpriteRenderSystem.h"

class RenderSystem
{
public:
	RenderSystem(Renderer& renderer, Camera2D& cam2D, Camera3D& cam3D) : renderer(renderer), cam2D(cam2D), cam3D(cam3D),
	                                                                     lights(), lightCBuffer(renderer, LightData{})
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

	void Register(LightComponent* lightComponent)
	{
		lights.push_back(lightComponent);
	}

	void Unregister(GameObject* object)
	{
		spriteSystem.Unregister(object);
		meshSystem.Unregister(object);

		std::erase_if(lights, [object](const LightComponent* lightComp)
		{
			return lightComp->owner == object;
		});
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
		
		LightData data = GetActiveLightData();
		data.camPos = cam3D.pos;
		lightCBuffer.Update(renderer, data);
		lightCBuffer.Bind(renderer);

		meshSystem.Render(renderer);
	}

	LightData GetActiveLightData()
	{
		for (const LightComponent* light: lights)
		{
			if (light->isEnabled)
			{
				return light->ToLightData();
			}
		}

		// fallback for no light situation
		LightData unlit{};
		unlit.ambient = { 0.3f, 0.3f, 0.3f };
		unlit.attConst = 1.f;
		return unlit;
	}

private:
	SpriteRenderSystem spriteSystem;
	MeshRenderSystem meshSystem;
	Renderer& renderer;

	Camera2D& cam2D;
	Camera3D& cam3D;

	std::vector<LightComponent*> lights;
	LightCBuffer lightCBuffer;
};
