#include "MeshRendererComponent.h"

#include "GameContext.h"
#include "GameObject.h"
#include "MaterialComponent.h"

void MeshRendererComponent::Render(Renderer& renderer)
{
	if (!isEnabled)
	{
		return;
	}

	MaterialComponent* material = owner->GetComponent<MaterialComponent>();
	if (!material)
	{
		return;
	}

	material->Bind(renderer);
	for (auto& mesh : owner->GetMeshes())
	{
		mesh->Draw(renderer);
	}
}

void MeshRendererComponent::OnAttach(GameContext& context)
{
	IComponent::OnAttach(context);

	context.renderSys.Register(this);
}
