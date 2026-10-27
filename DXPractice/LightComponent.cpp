#include "LightComponent.h"

#include "GameObject.h"
#include "Light.h"

LightData LightComponent::ToLightData() const
{
	LightData data{};

	data.lightPos = owner->GetTransform()->GetPosition();
	data.ambient = ambient;
	data.diffuseColor = color;
	data.diffuseIntensity = intensity;
	data.attConst = attConst;
	data.attLin = attLin;
	data.attQuad = attQuad;
	return data;
}

void LightComponent::OnAttach(GameContext& context)
{
	IComponent::OnAttach(context);

	context.renderSys.Register(this);
}
