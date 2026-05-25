#pragma once
#include "MaterialComponent.h"
#include "MeshComponent.h"
#include "TransformComponent.h"

class ModelReader;
class Renderer;

class GameObject
{
public:
	GameObject(Renderer& renderer, const MaterialData& matData, const ModelReader& modelReader);
	GameObject(Renderer& renderer, const std::vector<Vertex> vertices, const std::vector<unsigned int> indices);
	GameObject(Renderer& renderer);
	void Draw(Renderer& renderer);
	void Update(float deltaTime);

	// TODO: getcomponent
	TransformComponent* GetTransform();

private:
	TransformComponent transformComp;
	MaterialComponent materialComp;
	std::vector<std::unique_ptr<MeshComponent>> meshes;
};

