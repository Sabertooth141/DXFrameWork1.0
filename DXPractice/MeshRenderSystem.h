#pragma once
#include <deque>
#include <vector>

#include "MeshRendererComponent.h"

class MeshRenderSystem
{
public:
	void Register(MeshRendererComponent* meshRenderer)
	{
		meshRenderers.push_back(meshRenderer);
	}

	void Unregister(GameObject* object)
	{
		std::erase_if(meshRenderers, [object](const MeshRendererComponent* component)
		{
			return component->owner == object;
		});
	}

	void Render(Renderer& renderer) const
	{
		for (auto* meshRenderer : meshRenderers)
		{
			meshRenderer->Render(renderer);
		}
	}

	bool IsEmpty() const
	{
		return meshRenderers.empty();
	}

private:
	std::vector<MeshRendererComponent*> meshRenderers;
};
