#include "GameScene.h"

#include <format>

#include "SceneManager.h"
#include "TextRenderer.h"

void GameScene::OnEnter()
{
	Scene::OnEnter();

}

void GameScene::OnUpdate(float deltaTime)
{}

void GameScene::OnDrawUI(TextRenderer& text)
{
	Scene::OnDrawUI(text);
}
