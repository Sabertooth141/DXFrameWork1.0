#include "EditorUI.h"

#include <imgui.h>
#include <algorithm>

#include "AnimSetComponent.h"
#include "BoxCollider2D.h"
#include "EditorUtils.h"
#include "GameObject.h"
#include "HealthComponent.h"
#include "HitVolumeComponent.h"
#include "PrefabRegistry.h"
#include "Scene.h"
#include "SceneManager.h"

using EditorUtils::Is2DObject;
using EditorUtils::WakeObjRb;

namespace
{
	// 3d prefab placement
	constexpr float placeDistance = 5.f; // distance infront of camera for prefab placement

	DirectX::XMFLOAT3 Spawn3DInFront(const Camera3D& cam3D)
	{
		DirectX::XMFLOAT3 out;
		DirectX::XMStoreFloat3(&out, DirectX::XMVectorAdd(DirectX::XMLoadFloat3(&cam3D.pos),
		                                                  DirectX::XMVectorScale(
			                                                  cam3D.GetCamForward(), placeDistance)));
		return out;
	}
}

void EditorUI::Draw(SceneManager& sceneManager, GameContext& context)
{
	if (!isVisible)
	{
		return;
	}

	DrawToolbar(sceneManager);

	Scene* scene = sceneManager.GetCurrScene();
	if (!scene)
	{
		return;
	}

	ValidateSelection(*scene);

	const bool isTransitioning = sceneManager.IsTransitioning();
	ImGui::BeginDisabled(isTransitioning);
	DrawHierarchy(*scene);
	DrawInspector(*scene);
	DrawDebug(*scene);

	DrawPrefabPalette(*scene, context);
	ImGui::EndDisabled();

	DrawLayoutSave(sceneManager);
	viewport.DrawObjectIcons(*scene, context);

	if (!isTransitioning && CanEditViewport())
	{
		viewport.HandleFlyCamera(context.cam3D, context.keyboard, context.mouse);
	}
	else
	{
		viewport.StopFlying();
	}

	if (!isTransitioning)
	{
		if (CanEditViewport())
		{
			viewport.HandleMouse(*scene, context, selectedObject);
		}
		else
		{
			viewport.CancelDrag();
		}
	}

	if (selectedObject)
	{
		viewport.DrawSelectionOutline(*selectedObject, context);
	}
}

void EditorUI::ToggleVisible()
{
	isVisible = !isVisible;
	if (!isVisible)
	{
		isPaused = false;
		viewport.CancelDrag();
		stepRequested = false;
		viewport.StopFlying();
		return;
	}
	isPaused = true;
}

bool EditorUI::ShouldTick()
{
	if (mode == Mode::EDIT)
	{
		stepRequested = false;
		return false;
	}

	if (!isPaused)
	{
		return true;
	}

	if (stepRequested)
	{
		stepRequested = false;
		return true;
	}

	return false;
}

void EditorUI::ValidateSelection(Scene& scene)
{
	if (&scene != lastScene)
	{
		lastScene = &scene;
		selectedObject = nullptr;
		viewport.CancelDrag();
		return;
	}

	if (!selectedObject)
	{
		return;
	}

	const auto& objInScene = scene.GetObjects();
	const bool isObjAlive = std::ranges::any_of(objInScene, [this](const auto& obj)
	{
		return obj.get() == selectedObject;
	});

	if (!isObjAlive)
	{
		selectedObject = nullptr;
		viewport.CancelDrag();
	}
}

void EditorUI::DrawHierarchy(Scene& scene)
{
	ImGui::SetNextWindowPos({10, 110.f}, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize({200.f, 300.f}, ImGuiCond_FirstUseEver);
	if (ImGui::Begin("ヒエラルキー"))
	{
		ImGui::TextDisabled("%zu objects", scene.GetObjects().size());
		ImGui::Separator();

		for (auto& objPtr : scene.GetObjects())
		{
			GameObject* obj = objPtr.get();
			ImGui::PushID(obj);

			if (ImGui::Selectable(obj->GetName().c_str(), selectedObject == obj))
			{
				selectedObject = obj;
			}
			ImGui::PopID();
		}
	}

	ImGui::End();
}

void EditorUI::DrawInspector(Scene& scene)
{
	ImGui::SetNextWindowPos({540.f, 170.f}, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize({250.f, 420.f}, ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("インスペクター"))
	{
		ImGui::End();
		return;
	}

	if (!selectedObject)
	{
		ImGui::TextDisabled("オブジェクト未選択");
		ImGui::End();
		return;
	}

	GameObject& selectedObjRef = *selectedObject;
	ImGui::PushItemWidth(-90.f);

	// name & tag
	char nameBuffer[128];
	strncpy_s(nameBuffer, selectedObjRef.GetName().c_str(), _TRUNCATE);
	if (ImGui::InputText("名前", nameBuffer, sizeof(nameBuffer)))
	{
		selectedObjRef.SetName(nameBuffer);
	}

	int tag = static_cast<int>(selectedObjRef.GetTag());
	if (ImGui::Combo("タグ", &tag, objectTagNames, IM_ARRAYSIZE(objectTagNames)))
	{
		selectedObjRef.SetTag(static_cast<ObjectTag>(tag));
	}

	// transform
	if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
	{
		TransformComponent* transform = selectedObjRef.GetTransform();
		// 2D: pixel units on the XY plane. 3D: metres in XYZ. The data is three values either way
		const bool is2D = Is2DObject(selectedObjRef);

		DirectX::XMFLOAT3 pos = transform->GetPosition();
		const bool isPosChanged = is2D
			                          ? ImGui::DragFloat2("位置", &pos.x, 1.f)
			                          : ImGui::DragFloat3("位置", &pos.x, 0.05f);
		if (isPosChanged)
		{
			transform->SetPosition(pos);
			WakeObjRb(selectedObjRef, false);
		}

		// Stored in radians; only the display is converted to degrees (order: XMMatrixRotationRollPitchYaw(x, y, z))
		DirectX::XMFLOAT3 rot = transform->GetRotation();
		DirectX::XMFLOAT3 deg = {
			DirectX::XMConvertToDegrees(rot.x), DirectX::XMConvertToDegrees(rot.y), DirectX::XMConvertToDegrees(rot.z)
		};
		const bool isRotChanged = is2D
			                          ? ImGui::DragFloat("回転（度）", &deg.z, 0.5f) // 2D: Z only
			                          : ImGui::DragFloat3("回転（度）", &deg.x, 0.5f);
		if (isRotChanged)
		{
			rot = {
				DirectX::XMConvertToRadians(deg.x), DirectX::XMConvertToRadians(deg.y),
				DirectX::XMConvertToRadians(deg.z)
			};
			transform->SetRotation(rot);
			WakeObjRb(selectedObjRef, false);
		}

		DirectX::XMFLOAT3 scale = transform->GetScale();
		const bool isScaleChanged = is2D
			                            ? ImGui::DragFloat2("スケール", &scale.x, 0.5f)
			                            : ImGui::DragFloat3("スケール", &scale.x, 0.01f);
		if (isScaleChanged)
		{
			transform->SetScale(scale);
			WakeObjRb(selectedObjRef, false);
		}
	}

	// rigidbody
	if (auto* rb = selectedObjRef.GetComponent<Rigidbody2DComponent>())
	{
		if (ImGui::CollapsingHeader("Rigidbody2D", ImGuiTreeNodeFlags_DefaultOpen))
		{
			bool isStatic = rb->IsStatic();
			if (ImGui::Checkbox("Static", &isStatic))
			{
				rb->SetIsStatic(isStatic);
				rb->Wake();
			}

			bool isFreezeRot = rb->IsFreezeRotation();
			if (ImGui::Checkbox("回転を固定", &isFreezeRot))
			{
				rb->SetFreezeRotation(isFreezeRot);
			}

			DirectX::XMFLOAT2 vel = rb->GetVelocity();
			if (ImGui::DragFloat2("速度", &vel.x, 1.f))
			{
				rb->SetVelocity(vel);
				rb->Wake();
			}

			float angVel = rb->GetAngularVel();
			if (ImGui::DragFloat("角速度", &angVel, 0.05f))
			{
				rb->SetAngularVel(angVel);
				rb->Wake();
			}

			float mass = rb->GetMass();
			if (ImGui::DragFloat("質量", &mass, 0.05f, 0.01f, 10000.f))
			{
				mass = std::max(mass, 0.01f);
				rb->SetMass(mass);
				// 慣性モーメントは質量から計算されるので更新しておく
				if (auto* col = selectedObjRef.GetComponent<BoxCollider2D>())
				{
					rb->SetInertia(col->ComputeInertia(mass));
				}
			}

			float gravity = rb->GetGravity();
			if (ImGui::DragFloat("重力", &gravity, 5.f))
			{
				rb->SetGravity(gravity);
				rb->Wake();
			}

			float restitution = rb->GetRestitution();
			if (ImGui::SliderFloat("反発係数", &restitution, 0.f, 1.f))
			{
				rb->SetRestitution(restitution);
			}

			ImGui::Text("状態: %s", rb->IsSleeping() ? "スリープ" : "動作中");
			ImGui::SameLine();
			if (ImGui::SmallButton("Wake"))
			{
				rb->Wake();
			}
		}
	}

	// box collider
	if (auto* col = selectedObjRef.GetComponent<BoxCollider2D>())
	{
		if (ImGui::CollapsingHeader("BoxCollider2D", ImGuiTreeNodeFlags_DefaultOpen))
		{
			bool trigger = col->IsTrigger();
			if (ImGui::Checkbox("Trigger", &trigger))
			{
				col->SetTrigger(trigger);
			}

			bool active = col->IsColliderActive();
			if (ImGui::Checkbox("判定を有効", &active))
			{
				col->SetColliderActive(active);
			}
		}
	}

	// hit volumes
	if (auto* volume = selectedObjRef.GetComponent<HitVolumeComponent>())
	{
		hitVolumeInspector.Draw(selectedObjRef, *volume);
	}

	// animation data (clips / fades / events)
	if (auto* animSet = selectedObjRef.GetComponent<AnimSetComponent>())
	{
		animSetInspector.Draw(selectedObjRef, *animSet);
	}

	// Animator
	if (auto* anim = selectedObjRef.GetComponent<AnimatorComponent>())
	{
		if (ImGui::CollapsingHeader("Animator", ImGuiTreeNodeFlags_DefaultOpen))
		{
			bool enabled = anim->IsEnabled();
			if (ImGui::Checkbox("表示", &enabled))
			{
				anim->SetEnabled(enabled);
			}
			ImGui::Text("スクリーン空間: %s", anim->IsScreenSpaceEnabled() ? "ON" : "OFF");
		}
	}

	ImGui::PopItemWidth();

	ImGui::Separator();
	if (ImGui::Button("削除"))
	{
		scene.Destroy(selectedObject); // FlushPending で実際に削除される
		selectedObject = nullptr;
		viewport.CancelDrag();
	}

	ImGui::End();
}

void EditorUI::DrawToolbar(SceneManager& sceneManager)
{
	ImGui::SetNextWindowPos({540.f, 10.f}, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize({250.f, 150.f}, ImGuiCond_FirstUseEver);
	if (ImGui::Begin("エディター"))
	{
		ImGui::Text("%.1f fps", ImGui::GetIO().Framerate);

		const bool isTransitioning = sceneManager.IsTransitioning();

		ImGui::BeginDisabled(isTransitioning);
		if (mode == Mode::EDIT)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, {0.2f, 0.55f, 0.25f, 1.f});
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.2f, 0.65f, 0.3f, 1.f});
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, {0.15f, 0.45f, 0.2f, 1.f});

			const bool isPlayPressed = ImGui::Button("プレイ", {90.f, 0.f});
			ImGui::PopStyleColor(3);

			if (isPlayPressed)
			{
				EnterPlayMode(sceneManager);
			}

			ImGui::SameLine();
			ImGui::TextColored({0.6f, 0.85f, 1.f, 1.f}, "編集モード中");
		}
		else
		{
			ImGui::PushStyleColor(ImGuiCol_Button, {0.6f, 0.25f, 0.2f, 1.f});
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.7f, 0.3f, 0.25f, 1.f});
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, {0.5f, 0.2f, 0.15f, 1.f});

			const bool isStopPressed = ImGui::Button("編集モード", {90.f, 0.f});
			ImGui::PopStyleColor(3);

			if (isStopPressed)
			{
				EnterEditMode(sceneManager);
			}

			ImGui::SameLine();
			if (isPaused)
			{
				ImGui::TextColored({1.f, 0.7f, 0.3f, 1.f}, "プレイ中（一時停止）");
			}
			else
			{
				ImGui::TextColored({1.f, 0.7f, 0.3f, 1.f}, "プレイ中");
			}
		}
		ImGui::EndDisabled();

		ImGui::BeginDisabled(mode != Mode::PLAY);
		if (ImGui::Button(isPaused ? "再開" : "一時停止", {90.f, 0.f,}))
		{
			isPaused = !isPaused;
			viewport.CancelDrag();
		}

		ImGui::SameLine();
		ImGui::BeginDisabled(!isPaused);
		if (ImGui::Button("1フレーム進める"))
		{
			stepRequested = true;
		}
		ImGui::EndDisabled();
		ImGui::EndDisabled();

		ImGui::BeginDisabled(isTransitioning);
		const std::string currScene = sceneManager.GetCurrSceneName();
		if (ImGui::BeginCombo("シーン", currScene.c_str()))
		{
			for (const std::string& sceneName : sceneManager.GetSceneNames())
			{
				if (ImGui::Selectable(sceneName.c_str(), sceneName == currScene))
				{
					sceneManager.RequestChange(sceneName);
				}
			}
			ImGui::EndCombo();
		}

		if (ImGui::Button("リロード"))
		{
			sceneManager.RequestChange(currScene);
		}

		if (mode == Mode::EDIT && ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("保存していない変更は破棄されます");
		}

		ImGui::EndDisabled();

		ImGui::TextDisabled("F1：表示切替");
		if (mode == Mode::EDIT)
		{
			ImGui::TextDisabled("クリックで選択／ドラッグで移動");
		}
		else if (isPaused)
		{
			ImGui::TextDisabled("一時停止中：クリックで選択／ドラッグで移動");
		}
	}

	ImGui::End();
}

void EditorUI::DrawLayoutSave(SceneManager& sceneManager)
{
	ImGui::Begin("シーンセーブ");

	Scene* scene = sceneManager.GetCurrScene();
	const bool canSave = scene && mode == Mode::EDIT && !sceneManager.IsTransitioning();

	ImGui::BeginDisabled(!canSave);
	if (ImGui::Button("シーンを保存") && canSave)
	{
		const SceneLayout::Result result = SceneLayout::Save(*scene, sceneManager.GetCurrSceneName());
		saveValid = result.valid;
		saveMessage = result.message;
	}

	ImGui::EndDisabled();
	if (mode != Mode::EDIT)
	{
		ImGui::SameLine();
		ImGui::TextDisabled("編集モードのみ");
	}

	const auto drawStatus = [](bool isValid, const std::string& logMessage, bool highlight = false)
	{
		ImVec4 color = {1.f, 0.4f, 0.4f, 1.f};

		if (isValid)
		{
			if (highlight)
			{
				color = {0.4f, 0.9f, 0.4f, 1.f};
			}
			else
			{
				color = ImGui::GetStyle().Colors[ImGuiCol_TextDisabled];
			}
		}

		ImGui::PushStyleColor(ImGuiCol_Text, color);
		ImGui::TextWrapped("%s", logMessage.c_str());
		ImGui::PopStyleColor();
	};

	const SceneLayout::Result& load = sceneManager.GetLastLayoutLoad();
	if (!load.message.empty())
	{
		drawStatus(load.valid && load.skippedCount == 0, load.message);
	}

	if (!saveMessage.empty())
	{
		drawStatus(saveValid, saveMessage, true);
	}

	ImGui::End();
}

void EditorUI::DrawPrefabPalette(Scene& scene, GameContext& context)
{
	ImGui::SetNextWindowSize({200.f, 250.f}, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowPos({10.f, 420.f}, ImGuiCond_FirstUseEver);

	if (ImGui::Begin("プレハブ"))
	{
		const bool canPlace = mode == Mode::EDIT;
		if (canPlace)
		{
			ImGui::TextDisabled("クリックで配置（2D：画面中央／3D：カメラの前）");
		}
		else
		{
			ImGui::TextDisabled("編集モードのみ配置できます");
		}

		ImGui::Separator();

		ImGui::BeginDisabled(!canPlace);
		const PrefabRegistry& registry = PrefabRegistry::Instance();
		for (const std::string& prefabName : registry.GetPrefabNames())
		{
			const PrefabDef* def = registry.Find(prefabName);
			const bool is2DPrefab = def->shape == PrefabShape::SPRITE;

			const bool isClicked = ImGui::Selectable(prefabName.c_str());
			ImGui::SameLine();
			ImGui::TextDisabled(is2DPrefab ? "2D" : "3D");

			if (isClicked)
			{
				DirectX::XMFLOAT3 spawnPos;
				if (is2DPrefab)
				{
					const DirectX::XMFLOAT2 cam = context.cam2D.GetPosition();
					spawnPos = {cam.x, cam.y, 1.f};
				}
				else
				{
					spawnPos = Spawn3DInFront(context.cam3D);
				}

				// Added to the scene at the next frame's FlushPending (Flush also runs while paused)
				if (GameObject* obj = scene.InstantiateAsLayout(prefabName, spawnPos))
				{
					selectedObject = obj;
					viewport.CancelDrag();
				}
			}
		}
		ImGui::EndDisabled();
	}

	ImGui::End();
}

void EditorUI::DrawDebug(Scene& scene)
{
	ImGui::Begin("Debug Stats");

	ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
	ImGui::Text("objects %zu", scene.GetObjects().size());

	ImGui::SeparatorText("ObjectsHp");
	for (const auto& object : scene.GetObjects())
	{
		if (const HealthComponent* health = object->GetComponent<HealthComponent>())
		{
			ImGui::Text("%s HP %.0f / %.0f", object->GetName().c_str(), health->GetCurrHp(), health->GetMaxHp());
		}
	}

	ImGui::End();
}

void EditorUI::EnterPlayMode(SceneManager& sceneManager)
{
	Scene* scene = sceneManager.GetCurrScene();
	if (!scene || sceneManager.IsTransitioning())
	{
		return;
	}

	const std::string sceneName = sceneManager.GetCurrSceneName();
	const SceneLayout::Result& load = sceneManager.GetLastLayoutLoad();

	const bool hasLayoutObj = std::ranges::any_of(scene->GetObjects(), [scene](const auto& obj)
	{
		return obj->IsLayoutObject() && !scene->IsPendingDestroy(obj.get());
	});

	if (!load.valid || load.skippedCount > 0)
	{
		saveValid = false;
		saveMessage = "自動保存をスキップ（" + sceneName + "）：読み込みエラー／スキップがあるため、ファイルを上書きしません";
	}
	else if (load.fileFound || hasLayoutObj)
	{
		const SceneLayout::Result result = SceneLayout::Save(*scene, sceneName);
		saveValid = result.valid;
		saveMessage = result.message;

		if (!result.valid)
		{
			return;
		}
	}

	playModeStartScene = sceneName;
	mode = Mode::PLAY;
	isPaused = false;
	stepRequested = false;
	viewport.CancelDrag();
	selectedObject = nullptr;

	sceneManager.RequestChange(sceneName);
	viewport.StopFlying();
}

void EditorUI::EnterEditMode(SceneManager& sceneManager)
{
	if (sceneManager.IsTransitioning())
	{
		return;
	}

	mode = Mode::EDIT;
	isPaused = false;
	stepRequested = false;
	viewport.CancelDrag();
	selectedObject = nullptr;

	std::string tgtScene;

	if (sceneManager.IsRegistered(playModeStartScene))
	{
		tgtScene = playModeStartScene;
	}
	else
	{
		tgtScene = sceneManager.GetCurrSceneName();
	}

	sceneManager.RequestChange(tgtScene);
	viewport.StopFlying();
}
