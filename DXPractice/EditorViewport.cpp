#include "EditorViewport.h"

#include <imgui.h>
#include <cfloat>
#include <vector>

#include "Camera2D.h"
#include "EditorUtils.h"
#include "GameObject.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "Scene.h"
#include "WindowSettings.h"

using EditorUtils::Is2DObject;
using EditorUtils::IsObjectScreenSpace;
using EditorUtils::WakeObjRb;

namespace
{
	constexpr float iconRadius = 8.f;
	constexpr float nearClip = 0.1f;
	constexpr float maxDragDistance = 500.f; // distance before object becomes unresponsive to drag

	struct Ray
	{
		DirectX::XMVECTOR origin;
		DirectX::XMVECTOR dir;
	};

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

void EditorViewport::HandleMouse(Scene& scene, GameContext& context, GameObject*& selectedObject)
{
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
				objDragOffset = {pos.x - cursorWorldPos.x, pos.y - cursorWorldPos.y};
				isDragging = true;
			}
			else
			{
				isDragging = BeginDrag3D(*selectedObject, context, mouse);
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
		UpdateDrag3D(*selectedObject, context, mouse);
		return;
	}

	TransformComponent* transform = selectedObject->GetTransform();
	const DirectX::XMFLOAT3 pos = transform->GetPosition();
	const DirectX::XMFLOAT2 worldPos = ScreenToWorld(mouse, cam, IsObjectScreenSpace(*selectedObject));
	transform->SetPosition({worldPos.x + objDragOffset.x, worldPos.y + objDragOffset.y, pos.z});
	WakeObjRb(*selectedObject, true);
}

bool EditorViewport::BeginDrag3D(GameObject& object, GameContext& context, DirectX::XMFLOAT2 mouseScreenPos)
{
	const DirectX::XMFLOAT3 pos = object.GetTransform()->GetPosition();
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

void EditorViewport::UpdateDrag3D(GameObject& object, GameContext& context, DirectX::XMFLOAT2 mouseScreenPos)
{
	// if shift pressed or released -> rebuild normal / stop dragging
	if (ImGui::GetIO().KeyShift != isDragVertical && !BeginDrag3D(object, context, mouseScreenPos))
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
	object.GetTransform()->SetPosition(newPos);
	WakeObjRb(object, true);
}

void EditorViewport::HandleFlyCamera(Camera3D& cam, const Keyboard& keyboard, Mouse& mouse)
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

void EditorViewport::DrawSelectionOutline(GameObject& selectedObject, GameContext& context)
{
	ImDrawList* drawList = ImGui::GetBackgroundDrawList();
	const ImU32 color = IM_COL32(255, 200, 0, 255);

	if (Is2DObject(selectedObject))
	{
		DirectX::XMFLOAT2 corners[4];
		GetObjCorners(selectedObject, corners);

		const DirectX::XMFLOAT2 cam = context.cam2D.GetPosition();
		const bool isScreenSpace = IsObjectScreenSpace(selectedObject);

		drawList->AddQuad(WorldToScreen(corners[0], cam, isScreenSpace), WorldToScreen(corners[1], cam, isScreenSpace),
		                  WorldToScreen(corners[2], cam, isScreenSpace), WorldToScreen(corners[3], cam, isScreenSpace),
		                  IM_COL32(255, 200, 0, 255), 2.f);

		return;
	}

	DirectX::BoundingOrientedBox obb;
	if (selectedObject.GetWorldBounds(obb))
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

	if (WorldToScreen3D(selectedObject.GetTransform()->GetPosition(), context, screenPos))
	{
		drawList->AddCircle(screenPos, iconRadius + 4.f, color, 0, 2.f);
	}
}

void EditorViewport::DrawObjectIcons(Scene& scene, GameContext& context)
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

GameObject* EditorViewport::Pick(Scene& scene, GameContext& context, DirectX::XMFLOAT2 mousePosScreen)
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

GameObject* EditorViewport::Pick2D(Scene& scene, const Camera2D& camera, const DirectX::XMFLOAT2 mousePosScreen)
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

GameObject* EditorViewport::Pick3D(Scene& scene, GameContext& context, DirectX::XMFLOAT2 mousePosScreen)
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
