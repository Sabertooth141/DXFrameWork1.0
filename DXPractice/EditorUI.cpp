#include "EditorUI.h"

#include <imgui.h>

#include "BoxCollider2D.h"
#include "Camera2D.h"
#include "GameObject.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "PrefabRegistry.h"
#include "Scene.h"
#include "SceneManager.h"
#include "WindowSettings.h"

namespace
{
	constexpr float iconRadius = 8.f;
	constexpr float nearClip = 0.1f;

	// 3d prefab placement
	constexpr float placeDistance = 5.f; // distance infront of camera for prefab placement
	constexpr float maxDragDistance = 500.f; // distance before object becomes unresponsive to drag

	bool Is2DObject(GameObject& obj)
	{
		return obj.GetComponent<AnimatorComponent>() != nullptr;
	}

	struct Ray
	{
		DirectX::XMVECTOR origin;
		DirectX::XMVECTOR dir;
	};

	bool IsObjectScreenSpace(GameObject& obj)
	{
		auto* anim = obj.GetComponent<AnimatorComponent>();
		return anim && anim->IsScreenSpaceEnabled();
	}

	// screen: topleft x+ y+ to bottomright, world: x+ left to right, y+ bottom to top, z+ close to far
	DirectX::XMFLOAT2 ScreenToWorld(DirectX::XMFLOAT2 screen, DirectX::XMFLOAT2 cam, bool screenSpace)
	{
		const float x = screen.x - static_cast<float>(WIN_WIDTH) * 0.5f;
		const float y = static_cast<float>(WIN_HEIGHT) * 0.5f - screen.y;
		if (screenSpace)
		{
			return {x, y};
		}

		return {x + cam.x, y + cam.y};
	}

	bool IntersectRayPlane(const Ray& ray, DirectX::XMFLOAT3 planePoint, DirectX::XMFLOAT3 planeNormal,
	                       DirectX::XMVECTOR& outHit)
	{
		const DirectX::XMVECTOR normal = DirectX::XMLoadFloat3(&planeNormal);
		const float denominator = DirectX::XMVectorGetX(DirectX::XMVector3Dot(ray.dir, normal));
		// 
		if (std::abs(denominator) < 1e-3f)
		{
			return false;
		}

		const DirectX::XMVECTOR toPlane = DirectX::XMVectorSubtract(DirectX::XMLoadFloat3(&planePoint), ray.origin);
		const float projectedDist = DirectX::XMVectorGetX(DirectX::XMVector3Dot(toPlane, normal)) / denominator;

		if (projectedDist < 0.f || projectedDist > maxDragDistance)
		{
			return false;
		}

		outHit = DirectX::XMVectorAdd(ray.origin, DirectX::XMVectorScale(ray.dir, projectedDist));
		return true;
	}

	DirectX::XMFLOAT3 Spawn3DInFront(const Camera3D& cam3D)
	{
		DirectX::XMFLOAT3 out;
		DirectX::XMStoreFloat3(&out, DirectX::XMVectorAdd(DirectX::XMLoadFloat3(&cam3D.pos),
		                                                  DirectX::XMVectorScale(
			                                                  cam3D.GetCamForward(), placeDistance)));
		return out;
	}

	ImVec2 WorldToScreen(DirectX::XMFLOAT2 world, DirectX::XMFLOAT2 cam, bool screenSpace)
	{
		if (!screenSpace)
		{
			world.x -= cam.x;
			world.y -= cam.y;
		}
		return {world.x + WIN_WIDTH * 0.5f, WIN_HEIGHT * 0.5f - world.y};
	}

	// スプライトは -0.5〜0.5 の四角形を Transform でスケール・回転したもの
	void GetObjCorners(GameObject& obj, DirectX::XMFLOAT2 outCorners[4])
	{
		const TransformComponent* objTransform = obj.GetTransform();
		const DirectX::XMFLOAT3 objPos = objTransform->GetPosition();
		const DirectX::XMFLOAT3 objRot = objTransform->GetRotation();
		const DirectX::XMFLOAT3 objScale = objTransform->GetScale();

		const float halfX = objScale.x * 0.5f;
		const float halfY = objScale.y * 0.5f;
		const float rotCos = std::cos(objRot.z);
		const float rotSin = std::sin(objRot.z);

		const DirectX::XMFLOAT2 local[4] = {{-halfX, -halfY}, {halfX, -halfY}, {halfX, halfY}, {-halfX, halfY}};
		for (int i = 0; i < 4; ++i)
			outCorners[i] = {
				objPos.x + local[i].x * rotCos - local[i].y * rotSin,
				objPos.y + local[i].x * rotSin + local[i].y * rotCos
			};
	}

	bool ObjHitTest(GameObject& obj, DirectX::XMFLOAT2 cursorPos)
	{
		const TransformComponent* t = obj.GetTransform();
		const DirectX::XMFLOAT3 pos = t->GetPosition();
		const DirectX::XMFLOAT3 rot = t->GetRotation();
		const DirectX::XMFLOAT3 sc = t->GetScale();

		// クリック位置をオブジェクトのローカル空間へ
		const float dx = cursorPos.x - pos.x;
		const float dy = cursorPos.y - pos.y;
		const float c = std::cos(-rot.z);
		const float s = std::sin(-rot.z);
		const float lx = dx * c - dy * s;
		const float ly = dx * s + dy * c;

		return std::abs(lx) <= std::abs(sc.x) * 0.5f && std::abs(ly) <= std::abs(sc.y) * 0.5f;
	}

	// エディタから動かしたらスリープ中の剛体を起こす
	void WakeObjRb(GameObject& obj, bool stop)
	{
		if (auto* rb = obj.GetComponent<Rigidbody2DComponent>())
		{
			if (stop)
			{
				rb->SetVelocity({0.f, 0.f});
				rb->SetAngularVel(0.f);
			}

			rb->Wake();
		}
	}

	// raycast from screen to world
	Ray ScreenToRay(DirectX::XMFLOAT2 mousePos, GameContext& context)
	{
		const DirectX::XMMATRIX view = context.cam3D.GetView();
		const DirectX::XMMATRIX proj = context.renderer.Get3DProj();

		const float screenWidth = static_cast<float>(context.renderer.screenWidth);
		const float screenHeight = static_cast<float>(context.renderer.screenHeight);

		const DirectX::XMVECTOR nearPt = DirectX::XMVector3Unproject(
			DirectX::XMVectorSet(mousePos.x, mousePos.y, 0.f, 0.f), 0.f, 0.f, screenWidth, screenHeight, 0.f, 1.f,
			proj, view, DirectX::XMMatrixIdentity());
		const DirectX::XMVECTOR farPt = DirectX::XMVector3Unproject(
			DirectX::XMVectorSet(mousePos.x, mousePos.y, 1.f, 0.f), 0.f, 0.f, screenWidth, screenHeight, 0.f, 1.f,
			proj, view, DirectX::XMMatrixIdentity());

		return {nearPt, DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(farPt, nearPt))};
	}

	bool WorldToScreen3D(DirectX::XMFLOAT3 worldPos, GameContext& context, ImVec2& out)
	{
		const DirectX::XMMATRIX viewProj = context.cam3D.GetView() * context.renderer.Get3DProj();
		const DirectX::XMVECTOR clip = DirectX::XMVector4Transform(
			DirectX::XMVectorSet(worldPos.x, worldPos.y, worldPos.z, 1.f), viewProj);

		const float clipW = DirectX::XMVectorGetW(clip);

		// refuse anything close to near plane of clip space
		if (clipW < nearClip)
		{
			return false;
		}

		const float normDeviceCoordX = DirectX::XMVectorGetX(clip) / clipW; // -1 ... 1
		const float normDeviceCoordY = DirectX::XMVectorGetY(clip) / clipW; // -1 ... 1 (+y up)

		out =
		{
			(normDeviceCoordX * 0.5f + 0.5f) * static_cast<float>(context.renderer.screenWidth),
			(0.5f - normDeviceCoordY * 0.5f) * static_cast<float>(context.renderer.screenHeight) // +y down
		};
		return true;
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
	DrawPhysicsDebug(scene->GetPhysics());

	DrawPrefabPalette(*scene, context);
	ImGui::EndDisabled();

	DrawLayoutSave(sceneManager);
	DrawObjectIcon(*scene, context);

	if (!isTransitioning && CanEditViewport())
	{
		HandleEditorCamera(context.cam3D, context.keyboard, context.mouse);
	}
	else
	{
		isFlying = false;
	}

	if (!isTransitioning)
	{
		HandleViewportMouse(*scene, context);
	}

	if (selectedObject)
	{
		DrawSelectionOutline(context);
	}
}

void EditorUI::ToggleVisible()
{
	isVisible = !isVisible;
	if (!isVisible)
	{
		isPaused = false;
		isDragging = false;
		stepRequested = false;
		isFlying = false;
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
		isDragging = false;
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
		isDragging = false;
	}
}

bool EditorUI::BeginDrag3D(GameContext& context, DirectX::XMFLOAT2 mouseScreenPos)
{
	const DirectX::XMFLOAT3 pos = selectedObject->GetTransform()->GetPosition();
	isDragVertical = ImGui::GetIO().KeyShift;

	draggingPoint = pos;
	dragPlaneNormal = {0.f, 1.f, 0.f};
	if (isDragVertical)
	{
		const DirectX::XMVECTOR forward = context.cam3D.GetCamForward();
		const DirectX::XMVECTOR flat = DirectX::XMVectorSet(DirectX::XMVectorGetX(forward), 0.f,
		                                                    DirectX::XMVectorGetZ(forward), 0.f);
		const bool isLookingStraightDown = DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(flat)) < 1e-4f;
		DirectX::XMStoreFloat3(&dragPlaneNormal, DirectX::XMVector3Normalize(isLookingStraightDown ? forward : flat));
	}

	DirectX::XMVECTOR hit;
	if (!IntersectRayPlane(ScreenToRay(mouseScreenPos, context), draggingPoint, dragPlaneNormal, hit))
	{
		return false;
	}

	DirectX::XMStoreFloat3(&drag3DOffset, DirectX::XMVectorSubtract(DirectX::XMLoadFloat3(&pos), hit));
	return true;
}

void EditorUI::UpdateDrag3D(GameContext& context, DirectX::XMFLOAT2 mouseScreenPos)
{
	// if shift pressed or released -> rebuild normal / stop dragging
	if (ImGui::GetIO().KeyShift != isDragVertical && !BeginDrag3D(context, mouseScreenPos))
	{
		isDragging = false;
		return;
	}

	DirectX::XMVECTOR hit;
	if (!IntersectRayPlane(ScreenToRay(mouseScreenPos, context), draggingPoint, dragPlaneNormal, hit))
	{
		return;
	}

	DirectX::XMFLOAT3 newPos;
	DirectX::XMStoreFloat3(&newPos, DirectX::XMVectorAdd(hit, DirectX::XMLoadFloat3(&drag3DOffset)));
	selectedObject->GetTransform()->SetPosition(newPos);
	WakeObjRb(*selectedObject, true);
}

void EditorUI::HandleEditorCamera(Camera3D& cam, const Keyboard& keyboard, Mouse& mouse)
{
	const ImGuiIO& io = ImGui::GetIO();
	if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && !io.WantCaptureMouse)
	{
		isFlying = true;
		mouse.ConsumeRawDelta();
	}
	if (!ImGui::IsMouseDown(ImGuiMouseButton_Right))
	{
		isFlying = false;
	}

	if (!isFlying)
	{
		return;
	}

	const Mouse::RawDelta delta = mouse.ConsumeRawDelta();
	cam.Rotate(static_cast<float>(delta.x) * lookSens, static_cast<float>(-delta.y) * lookSens);

	const DirectX::XMVECTOR forward = cam.GetCamForward();
	const DirectX::XMVECTOR right = cam.GetCamRight();
	const DirectX::XMVECTOR up = DirectX::XMVectorSet(0.f, 1.f, 0.f, 0.f);

	DirectX::XMVECTOR move = DirectX::XMVectorZero();
	if (keyboard.KeyIsPressed('W'))
	{
		move = DirectX::XMVectorAdd(move, forward);
	}
	if (keyboard.KeyIsPressed('S'))
	{
		move = DirectX::XMVectorSubtract(move, forward);
	}
	if (keyboard.KeyIsPressed('D'))
	{
		move = DirectX::XMVectorAdd(move, right);
	}
	if (keyboard.KeyIsPressed('A'))
	{
		move = DirectX::XMVectorSubtract(move, right);
	}
	if (keyboard.KeyIsPressed(VK_SPACE))
	{
		move = DirectX::XMVectorAdd(move, up);
	}
	if (keyboard.KeyIsPressed(VK_CONTROL))
	{
		move = DirectX::XMVectorSubtract(move, up);
	}

	if (DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(move)) < 1e-6f)
	{
		return;
	}

	// normalize
	float speed = flySpeed;
	if (keyboard.KeyIsPressed(VK_SHIFT))
	{
		speed *= 3.f;
	}

	move = DirectX::XMVectorScale(DirectX::XMVector3Normalize(move), speed * io.DeltaTime);
	DirectX::XMStoreFloat3(&cam.pos, DirectX::XMVectorAdd(DirectX::XMLoadFloat3(&cam.pos), move));
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
		isDragging = false;
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
			isDragging = false;
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

void EditorUI::HandleViewportMouse(Scene& scene, GameContext& context)
{
	// if is in selection / is dragging -> return
	if (!CanEditViewport())
	{
		isDragging = false;
		return;
	}

	const ImGuiIO& io = ImGui::GetIO();
	const DirectX::XMFLOAT2 mouse = {io.MousePos.x, io.MousePos.y};
	const DirectX::XMFLOAT2 cam = context.cam2D.GetPosition();

	if (!io.WantCaptureMouse && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
	{
		selectedObject = Pick(scene, context, mouse);
		isDragging = false;
		if (selectedObject)
		{
			if (Is2DObject(*selectedObject))
			{
				const DirectX::XMFLOAT3 pos = selectedObject->GetTransform()->GetPosition();
				const DirectX::XMFLOAT2 cursorWorldPos = ScreenToWorld(mouse, cam, IsObjectScreenSpace(*selectedObject));
				objDragOffset = { pos.x - cursorWorldPos.x, pos.y - cursorWorldPos.y };
				isDragging = true;
			}
			else
			{
				isDragging = BeginDrag3D(context, mouse);
			}
		}
	}

	if (!isDragging)
	{
		return;
	}

	if (!selectedObject || !ImGui::IsMouseDown(ImGuiMouseButton_Left))
	{
		isDragging = false;
		return;
	}

	if (!Is2DObject(*selectedObject))
	{
		UpdateDrag3D(context, mouse);
		return;
	}

	TransformComponent* transform = selectedObject->GetTransform();
	const DirectX::XMFLOAT3 pos = transform->GetPosition();
	const DirectX::XMFLOAT2 worldPos = ScreenToWorld(mouse, cam, IsObjectScreenSpace(*selectedObject));
	transform->SetPosition({worldPos.x + objDragOffset.x, worldPos.y + objDragOffset.y, pos.z});
	WakeObjRb(*selectedObject, true);
}

void EditorUI::DrawSelectionOutline(GameContext& context)
{
	ImDrawList* drawList = ImGui::GetBackgroundDrawList();
	const ImU32 color = IM_COL32(255, 200, 0, 255);

	if (Is2DObject(*selectedObject))
	{
		DirectX::XMFLOAT2 corners[4];
		GetObjCorners(*selectedObject, corners);

		const DirectX::XMFLOAT2 cam = context.cam2D.GetPosition();
		const bool isScreenSpace = IsObjectScreenSpace(*selectedObject);

		drawList->AddQuad(WorldToScreen(corners[0], cam, isScreenSpace), WorldToScreen(corners[1], cam, isScreenSpace),
		                  WorldToScreen(corners[2], cam, isScreenSpace), WorldToScreen(corners[3], cam, isScreenSpace),
		                  IM_COL32(255, 200, 0, 255), 2.f);

		return;
	}

	DirectX::BoundingOrientedBox obb;
	if (selectedObject->GetWorldBounds(obb))
	{
		DirectX::XMFLOAT3 corners[DirectX::BoundingOrientedBox::CORNER_COUNT];
		obb.GetCorners(corners);

		static constexpr int edges[12][2] = {
			{0, 1}, {1, 2}, {2, 3}, {3, 0},
			{4, 5}, {5, 6}, {6, 7}, {7, 4},
			{0, 4}, {1, 5}, {2, 6}, {3, 7}
		};

		for (const auto& edge : edges)
		{
			ImVec2 lineStartPos, lineFinishPos;

			if (WorldToScreen3D(corners[edge[0]], context, lineStartPos) && WorldToScreen3D(
				corners[edge[1]], context, lineFinishPos))
			{
				drawList->AddLine(lineStartPos, lineFinishPos, color, 2.f);
			}
		}

		return;
	}

	// no mesh
	ImVec2 screenPos;

	if (WorldToScreen3D(selectedObject->GetTransform()->GetPosition(), context, screenPos))
	{
		drawList->AddCircle(screenPos, iconRadius + 4.f, color, 0, 2.f);
	}
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
	ImGui::SetNextWindowSize({ 200.f, 250.f }, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowPos({ 10.f, 420.f }, ImGuiCond_FirstUseEver);

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
					spawnPos = { cam.x, cam.y, 1.f };
				}
				else
				{
					spawnPos = Spawn3DInFront(context.cam3D);
				}

				// Added to the scene at the next frame's FlushPending (Flush also runs while paused)
				if (GameObject* obj = scene.InstantiateAsLayout(prefabName, spawnPos))
				{
					selectedObject = obj;
					isDragging = false;
				}
			}
		}
		ImGui::EndDisabled();
	}

	ImGui::End();
}

void EditorUI::DrawPhysicsDebug(PhysicsSystem& physics)
{
	const PhysicsSystem::DebugStats& s = physics.GetDebugStats();
	const int steps = physics.ConsumeStepCount();

	ImGui::Begin("Physics Stats");

	ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
	ImGui::Text("Physics steps this frame: %d", steps);

	ImGui::SeparatorText("Counts");
	ImGui::Text("Bodies:          %d", s.bodies);
	ImGui::Text("Grid cells:      %d", s.gridCells);
	ImGui::Text("Max per cell:    %d", s.maxPerCell);
	ImGui::Text("Raw pair checks: %d", s.rawPairChecks);
	ImGui::Text("Candidate pairs: %d", s.candidatePairs);
	ImGui::Text("Overlaps:        %d", s.overlaps);
	ImGui::Text("Contacts:        %d", s.contacts);

	ImGui::SeparatorText("Time per step (ms)");
	ImGui::Text("Integrate:       %.3f", s.integrateMs);
	ImGui::Text("Broad phase:     %.3f", s.broadPhaseMs);
	ImGui::Text("Narrow phase:    %.3f", s.narrowPhaseMs);
	ImGui::Text("Solve:           %.3f", s.solveMs);
	ImGui::Text("Positions:       %.3f", s.positionMs);
	ImGui::Text("Sleep + events:  %.3f", s.sleepAndEventsMs);
	ImGui::Text("Total:           %.3f", s.totalMs);

	ImGui::End();
}

void EditorUI::DrawObjectIcon(Scene& scene, GameContext& context)
{
	// switch drawing to background behind panels
	ImDrawList* drawList = ImGui::GetBackgroundDrawList();

	for (auto& objPtr : scene.GetObjects())
	{
		GameObject& obj = *objPtr;
		if (Is2DObject(obj) || obj.HasLocalBounds())
		{
			continue;
		}

		ImVec2 screenPos;
		if (!WorldToScreen3D(obj.GetTransform()->GetPosition(), context, screenPos))
		{
			continue;
		}

		constexpr ImU32 color = IM_COL32(200, 200, 200, 220);

		drawList->AddCircleFilled(screenPos, iconRadius * 0.6f, color);
		drawList->AddCircle(screenPos, iconRadius, color, 0, 1.5f);
		drawList->AddText({screenPos.x + iconRadius + 3.f, screenPos.y - 8.f}, color, obj.GetName().c_str());
	}
}

GameObject* EditorUI::Pick(Scene& scene, GameContext& context, DirectX::XMFLOAT2 mousePosScreen)
{
	if (GameObject* hit = Pick2D(scene, context.cam2D, mousePosScreen))
	{
		return hit;
	}
	else
	{
		return Pick3D(scene, context, mousePosScreen);
	}
}

GameObject* EditorUI::Pick2D(Scene& scene, const Camera2D& camera, const DirectX::XMFLOAT2 mousePosScreen)
{
	const DirectX::XMFLOAT2 camPos = camera.GetPosition();
	std::vector<GameObject*> candidates;
	for (auto& obj : scene.GetObjects())
	{
		if (Is2DObject(*obj))
		{
			candidates.push_back(obj.get());
		}
	}

	// topmost first: higher layer first, then higher sort order
	std::ranges::stable_sort(candidates, [](GameObject* a, GameObject* b)
	{
		auto* animA = a->GetComponent<AnimatorComponent>();
		auto* animB = b->GetComponent<AnimatorComponent>();

		const int layerA = animA ? static_cast<int>(animA->GetRenderLayer()) : 0;
		const int layerB = animB ? static_cast<int>(animB->GetRenderLayer()) : 0;

		if (layerA != layerB)
		{
			return layerA > layerB;
		}

		const int orderA = animA ? animA->GetSortOrder() : 0;
		const int orderB = animB ? animB->GetSortOrder() : 0;
		return orderA > orderB;
	});

	for (int pass = 0; pass < 2; pass++)
	{
		const bool checkScreenSpace = pass == 0;

		for (const auto obj : candidates)
		{
			const bool isScreenSpace = IsObjectScreenSpace(*obj);
			if (isScreenSpace != checkScreenSpace)
			{
				continue;
			}

			if (ObjHitTest(*obj, ScreenToWorld(mousePosScreen, camPos, isScreenSpace)))
			{
				return obj;
			}
		}
	}

	return nullptr;
}

GameObject* EditorUI::Pick3D(Scene& scene, GameContext& context, DirectX::XMFLOAT2 mousePosScreen)
{
	const Ray ray = ScreenToRay(mousePosScreen, context);

	GameObject* nearestObj = nullptr;
	float nearestDist = FLT_MAX;

	for (auto& objPtr : scene.GetObjects())
	{
		GameObject& obj = *objPtr;

		if (Is2DObject(obj))
		{
			continue;
		}

		// for objects with mesh
		DirectX::BoundingOrientedBox obb;
		if (obj.GetWorldBounds(obb))
		{
			float dist = 0.f;
			if (obb.Intersects(ray.origin, ray.dir, dist) && dist < nearestDist)
			{
				nearestObj = &obj;
				nearestDist = dist;
			}
			continue;
		}

		// for empty objects
		const DirectX::XMFLOAT3 pos = obj.GetTransform()->GetPosition();
		ImVec2 iconPos;
		if (!WorldToScreen3D(pos, context, iconPos))
		{
			continue;
		}

		const float dx = iconPos.x - mousePosScreen.x;
		const float dy = iconPos.y - mousePosScreen.y;

		if (dx * dx + dy * dy > iconRadius * iconRadius)
		{
			continue;
		}

		// check dist with mesh objects
		const float dist = DirectX::XMVectorGetX(
			DirectX::XMVector3Length(DirectX::XMVectorSubtract(DirectX::XMLoadFloat3(&pos), ray.origin)));

		if (dist < nearestDist)
		{
			nearestObj = &obj;
			nearestDist = dist;
		}
	}

	return nearestObj;
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
	isDragging = false;
	selectedObject = nullptr;

	sceneManager.RequestChange(sceneName);
	isFlying = false;
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
	isDragging = false;
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
	isFlying = false;
}
