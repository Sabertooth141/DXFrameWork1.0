#include "HitVolumeInspector.h"

#include <imgui.h>
#include <algorithm>
#include <cfloat>

#include "EditorUtils.h"
#include "GameObject.h"
#include "HitVolumeComponent.h"
#include "SkeletalAnimatorComponent.h"

using EditorUtils::ContainsNoCase;
using EditorUtils::InputString;

namespace
{
	// order = HitVolumeRole
	const char* hitRoleNames[] = {"Hurtbox", "Hitbox", "Body"};
}

void HitVolumeInspector::Draw(GameObject& object, HitVolumeComponent& volume)
{
	if (currObject != &object)
	{
		currObject = &object;
		selectedShape = -1;
		boneFilter[0] = '\0';
		message = volume.GetLoadMessage();
		isMessageOk = true;
	}

	if (!ImGui::CollapsingHeader("HitVolume", ImGuiTreeNodeFlags_DefaultOpen))
	{
		volume.highlightedShape = -1;
		return;
	}
	// the inspector already has "名前" / "削除" widgets: separate ID scope so they do not collide
	ImGui::PushID("HitVolume");

	// Edit a copy of the data and rebuild the component when anything changed.
	// Rebuilding everything is cheap (a few dozen shapes) and keeps one code path:
	// the editor, file loading and the code fallback all go through Build().
	HitVolumeDesc desc = volume.ExportDesc();
	bool isChanged = false;

	// file
	const std::string& path = volume.GetDataPath();
	ImGui::TextDisabled("%s", path.empty() ? "（ファイルなし：コードで定義）" : path.c_str());
	ImGui::BeginDisabled(path.empty());
	if (ImGui::Button("保存"))
	{
		isMessageOk = volume.SaveFile(message);
	}
	ImGui::SameLine();
	if (ImGui::Button("元に戻す"))
	{
		isMessageOk = volume.ReloadFile(message);
		selectedShape = -1;
		desc = volume.ExportDesc();
	}
	ImGui::EndDisabled();
	EditorUtils::StatusText(message, isMessageOk);
	ImGui::TextDisabled("F2 でデバッグ表示（選択中 = 黄、ボーンなし = 紫）");

	// add
	const auto addShape = [&](HitShapeDesc shape)
	{
		desc.shapes.push_back(std::move(shape));
		selectedShape = static_cast<int>(desc.shapes.size()) - 1;
		isChanged = true;
	};
	if (ImGui::Button("+ Hurtbox"))
	{
		addShape({.role = HitVolumeRole::HURTBOX, .name = "hurtbox", .radius = 0.3f});
	}
	ImGui::SameLine();
	if (ImGui::Button("+ Hitbox"))
	{
		addShape({.role = HitVolumeRole::HITBOX, .name = "hitbox", .groupName = "new", .radius = 0.3f});
	}
	ImGui::SameLine();
	if (ImGui::Button("+ Body"))
	{
		addShape({.role = HitVolumeRole::BODY, .name = "body", .radius = 0.5f});
	}

	// groups (settings shared by every hitbox in the group)
	if (!desc.groups.empty() && ImGui::TreeNode("グループ（再ヒット・ダメージ）"))
	{
		for (HitGroupDesc& group : desc.groups)
		{
			ImGui::PushID(group.name.c_str());
			ImGui::SeparatorText(group.name.c_str());
			isChanged |= ImGui::DragFloat("再ヒット間隔", &group.rehitIntervalSec, 0.01f, 0.f, 10.f, "%.2f s");
			isChanged |= ImGui::DragFloat("ダメージ", &group.damageScale, 1.f, 0.f, 10000.f, "%.0f");
			isChanged |= ImGui::DragFloat("スタッガー", &group.staggerScale, 1.f, 0.f, 10000.f, "%.0f");

			DirectX::XMFLOAT4 vfxColor = group.vfxColor;
			if (ImGui::ColorEdit4("VFX 色", &vfxColor.x, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_Float))
			{
				volume.SetGroupVFXColor(group.name, vfxColor); // in place: no rebuild, an active glow stays visible
				group.vfxColor = vfxColor; // keep this frame's desc in step in case a later widget rebuilds
			}
			ImGui::PopID();
		}
		ImGui::TextDisabled("0 = 1 回の発動で 1 回だけヒット");
		ImGui::TextDisabled("VFX 色の α = 0 で発光なし");
		ImGui::TreePop();
	}

	// shape list
	const std::vector<HitShape>& runtimeShapes = volume.GetShapes(); // same order as desc.shapes
	if (ImGui::BeginListBox("##shapes", {-FLT_MIN, 8.f * ImGui::GetTextLineHeightWithSpacing()}))
	{
		for (int i = 0; i < static_cast<int>(desc.shapes.size()); i++)
		{
			const HitShapeDesc& shape = desc.shapes[i];
			const bool isBoneMissing = i < static_cast<int>(runtimeShapes.size()) && runtimeShapes[i].isBoneMissing;

			char label[192];
			sprintf_s(label, "%-7s %s  (%s)", hitRoleNames[static_cast<int>(shape.role)], shape.name.c_str(),
			          shape.boneName.empty() ? "ルート" : shape.boneName.c_str());

			ImGui::PushID(i);
			if (isBoneMissing)
			{
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.4f, 1.f, 1.f));
			}
			if (ImGui::Selectable(label, i == selectedShape))
			{
				selectedShape = i == selectedShape ? -1 : i;
			}
			if (isBoneMissing)
			{
				ImGui::PopStyleColor();
			}
			ImGui::PopID();
		}
		ImGui::EndListBox();
	}

	// selected shape
	if (selectedShape >= static_cast<int>(desc.shapes.size()))
	{
		selectedShape = -1;
	}

	int removeIndex = -1;
	if (selectedShape >= 0)
	{
		HitShapeDesc& shape = desc.shapes[selectedShape];
		ImGui::SeparatorText("選択中のシェイプ");

		int role = static_cast<int>(shape.role);
		if (ImGui::Combo("種類", &role, hitRoleNames, IM_ARRAYSIZE(hitRoleNames)))
		{
			shape.role = static_cast<HitVolumeRole>(role);
			isChanged = true;
		}

		isChanged |= InputString("名前", shape.name);

		// group: type a new name, or pick an existing one from the arrow
		if (shape.role == HitVolumeRole::HITBOX)
		{
			isChanged |= InputString("グループ", shape.groupName);
			ImGui::SameLine();
			if (ImGui::BeginCombo("##groupPick", nullptr, ImGuiComboFlags_NoPreview))
			{
				for (const HitGroupDesc& group : desc.groups)
				{
					if (ImGui::Selectable(group.name.c_str(), group.name == shape.groupName))
					{
						shape.groupName = group.name;
						isChanged = true;
					}
				}
				ImGui::EndCombo();
			}
		}

		// bone: filterable list of every skeleton node
		const auto* animator = object.GetComponent<SkeletalAnimatorComponent>();
		const char* bonePreview = shape.boneName.empty() ? "（ルート）" : shape.boneName.c_str();
		if (ImGui::BeginCombo("ボーン", bonePreview, ImGuiComboFlags_HeightLarge))
		{
			if (ImGui::IsWindowAppearing())
			{
				ImGui::SetKeyboardFocusHere();
			}
			ImGui::InputTextWithHint("##boneFilter", "検索", boneFilter, sizeof(boneFilter));

			if (ImGui::Selectable("（ルート）", shape.boneName.empty()))
			{
				shape.boneName.clear();
				isChanged = true;
			}
			if (animator)
			{
				for (const SkeletonNode& node : animator->GetSkeleton().nodes)
				{
					if (!ContainsNoCase(node.name, boneFilter))
					{
						continue;
					}
					if (ImGui::Selectable(node.name.c_str(), node.name == shape.boneName))
					{
						shape.boneName = node.name;
						isChanged = true;
					}
				}
			}
			ImGui::EndCombo();
		}
		if (selectedShape < static_cast<int>(runtimeShapes.size()) && runtimeShapes[selectedShape].isBoneMissing)
		{
			ImGui::TextColored({1.f, 0.4f, 1.f, 1.f}, "ボーン \"%s\" が見つかりません", shape.boneName.c_str());
		}

		// offsets are in the bone's local frame (meters), shown as the axes in the viewport
		isChanged |= ImGui::DragFloat3("A", &shape.localCenterA.x, 0.01f, 0.f, 0.f, "%.3f");
		isChanged |= ImGui::DragFloat3("B", &shape.localCenterB.x, 0.01f, 0.f, 0.f, "%.3f");
		if (ImGui::SmallButton("B = A（球）"))
		{
			shape.localCenterB = shape.localCenterA;
			isChanged = true;
		}
		isChanged |= ImGui::DragFloat("半径", &shape.radius, 0.005f, 0.01f, 20.f, "%.3f");
		shape.radius = std::max(shape.radius, 0.01f);

		if (shape.role == HitVolumeRole::HITBOX)
		{
			isChanged |= ImGui::Checkbox("スイープ（速い攻撃）", &shape.isSweeping);
		}

		if (ImGui::Button("複製"))
		{
			HitShapeDesc copy = shape; // copy before push_back: it may reallocate
			addShape(std::move(copy));
		}
		ImGui::SameLine();
		if (ImGui::Button("削除"))
		{
			removeIndex = selectedShape;
		}
	}

	if (removeIndex >= 0)
	{
		desc.shapes.erase(desc.shapes.begin() + removeIndex);
		selectedShape = -1;
		isChanged = true;
	}

	if (isChanged)
	{
		volume.Build(desc);
	}
	volume.highlightedShape = selectedShape;

	ImGui::PopID();
}
