#include "AnimSetInspector.h"

#include <imgui.h>
#include <algorithm>
#include <cfloat>

#include "AnimSetComponent.h"
#include "EditorUtils.h"
#include "GameObject.h"
#include "HitVolumeComponent.h"
#include "SkeletalAnimatorComponent.h"

using EditorUtils::InputString;

void AnimSetInspector::Draw(GameObject& object, AnimSetComponent& animSet)
{
	auto* animator = object.GetComponent<SkeletalAnimatorComponent>();
	if (!animator)
	{
		return;
	}

	if (currObject != &object)
	{
		currObject = &object;
		selectedFade = -1;
		selectedEvent = -1;
		eventClipFilter.clear();
		message = animSet.GetLoadMessage();
		isMessageOk = true;
	}

	if (!ImGui::CollapsingHeader("Animation", ImGuiTreeNodeFlags_DefaultOpen))
	{
		return;
	}
	ImGui::PushID("AnimSet");

	// Edited in place on the component's desc; Apply() pushes it to the animator once at the end.
	// (Unlike the hit volume, the desc lives in the component: the animator cannot export it back.)
	AnimSetDesc& desc = animSet.GetDesc();
	bool isChanged = false;
	const std::vector<std::string> clipNames = animator->GetClipNames(); // sorted

	// clip picker; "*" (any) is allowed for fades only
	const auto clipCombo = [&](const char* label, std::string& value, bool isAnyAllowed)
	{
		bool isPicked = false;
		const char* preview = value == AnimationFadeTable::any ? "（全て）" : value.c_str();
		if (ImGui::BeginCombo(label, preview, ImGuiComboFlags_HeightLarge))
		{
			if (isAnyAllowed && ImGui::Selectable("（全て）", value == AnimationFadeTable::any))
			{
				value = AnimationFadeTable::any;
				isPicked = true;
			}
			for (const std::string& name : clipNames)
			{
				if (ImGui::Selectable(name.c_str(), name == value))
				{
					value = name;
					isPicked = true;
				}
			}
			ImGui::EndCombo();
		}
		return isPicked;
	};

	// file
	const std::string& path = animSet.GetDataPath();
	ImGui::TextDisabled("%s", path.empty() ? "（ファイルなし：コードで定義）" : path.c_str());
	ImGui::BeginDisabled(path.empty());
	if (ImGui::Button("保存"))
	{
		isMessageOk = animSet.SaveFile(message);
	}
	ImGui::SameLine();
	if (ImGui::Button("元に戻す"))
	{
		isMessageOk = animSet.ReloadFile(message);
		selectedFade = -1;
		selectedEvent = -1;
	}
	ImGui::EndDisabled();
	EditorUtils::StatusText(message, isMessageOk);

	// what is playing now: the reference point for placing events
	const std::string& currClip = animator->GetCurrClipName();
	const float currTime = animator->GetCurrClipTime();
	ImGui::Text("再生中: %s  %.3f / %.3f s", currClip.c_str(), currTime, animator->GetCurrClipLength());
	if (animator->IsExtractingRootYaw())
	{
		ImGui::Text("ルートのヨー（抽出）: %+.1f deg", DirectX::XMConvertToDegrees(animator->GetExtractedYawTotal()));
	}

	// ---- clips
	if (ImGui::TreeNode("クリップ"))
	{
		if (ImGui::Button("フォルダから追加"))
		{
			const int added = animSet.AddClipsFromFolder();
			message = std::to_string(added) + " 個のクリップを追加（保存で反映）";
			isMessageOk = true;
		}
		ImGui::SameLine();
		ImGui::TextDisabled("%s", desc.animDir.c_str());

		clipCombo("最初のクリップ", desc.initialClip, false);

		ImGui::TextDisabled("ループ / 再生");
		for (AnimClipDesc& clip : desc.clips)
		{
			ImGui::PushID(clip.name.c_str());
			isChanged |= ImGui::Checkbox("##loop", &clip.isLoop);
			ImGui::SameLine();
			ImGui::BeginDisabled(!animator->HasClip(clip.name)); // in the file but FBX missing
			if (ImGui::SmallButton("再生"))
			{
				animator->PlayClip(clip.name);
			}
			ImGui::EndDisabled();
			ImGui::SameLine();
			ImGui::Text("%s  (%.2f s)", clip.name.c_str(), animator->GetClipLength(clip.name));
			ImGui::PopID();
		}
		ImGui::TreePop();
	}

	// ---- fades
	if (ImGui::TreeNode("フェード"))
	{
		isChanged |= ImGui::DragFloat("既定", &desc.defaultFadeSec, 0.005f, 0.f, 2.f, "%.3f s");
		ImGui::TextDisabled("検索順: from→to、全て→to、from→全て、既定");

		if (ImGui::BeginListBox("##fades", {-FLT_MIN, 7.f * ImGui::GetTextLineHeightWithSpacing()}))
		{
			for (int i = 0; i < static_cast<int>(desc.fades.size()); i++)
			{
				const AnimFadeDesc& fade = desc.fades[i];
				char label[160];
				sprintf_s(label, "%s → %s   %.3f s", fade.from == AnimationFadeTable::any ? "（全て）" : fade.from.c_str(),
				          fade.to == AnimationFadeTable::any ? "（全て）" : fade.to.c_str(), fade.sec);
				ImGui::PushID(i);
				if (ImGui::Selectable(label, i == selectedFade))
				{
					selectedFade = i == selectedFade ? -1 : i;
				}
				ImGui::PopID();
			}
			ImGui::EndListBox();
		}

		if (ImGui::Button("+ フェード"))
		{
			desc.fades.push_back({AnimationFadeTable::any, currClip.empty() ? "*" : currClip, desc.defaultFadeSec});
			selectedFade = static_cast<int>(desc.fades.size()) - 1;
			isChanged = true;
		}

		if (selectedFade >= static_cast<int>(desc.fades.size()))
		{
			selectedFade = -1;
		}
		if (selectedFade >= 0)
		{
			AnimFadeDesc& fade = desc.fades[selectedFade];
			ImGui::SeparatorText("選択中のフェード");
			isChanged |= clipCombo("from", fade.from, true);
			isChanged |= clipCombo("to", fade.to, true);
			isChanged |= ImGui::DragFloat("時間", &fade.sec, 0.005f, 0.f, 2.f, "%.3f s");
			if (ImGui::Button("削除##fade"))
			{
				desc.fades.erase(desc.fades.begin() + selectedFade);
				selectedFade = -1;
				isChanged = true;
			}
		}
		ImGui::TreePop();
	}

	// ---- events
	if (ImGui::TreeNode("イベント"))
	{
		// filter by clip: with 40+ clips the full list is unreadable
		const char* filterPreview = eventClipFilter.empty() ? "（全て）" : eventClipFilter.c_str();
		if (ImGui::BeginCombo("表示", filterPreview, ImGuiComboFlags_HeightLarge))
		{
			if (ImGui::Selectable("（全て）", eventClipFilter.empty()))
			{
				eventClipFilter.clear();
			}
			if (!currClip.empty() && ImGui::Selectable("（再生中のクリップ）"))
			{
				eventClipFilter = currClip;
			}
			for (const std::string& name : clipNames)
			{
				if (ImGui::Selectable(name.c_str(), name == eventClipFilter))
				{
					eventClipFilter = name;
				}
			}
			ImGui::EndCombo();
		}

		if (ImGui::BeginListBox("##events", {-FLT_MIN, 8.f * ImGui::GetTextLineHeightWithSpacing()}))
		{
			for (int i = 0; i < static_cast<int>(desc.events.size()); i++)
			{
				const AnimEventDesc& event = desc.events[i];
				if (!eventClipFilter.empty() && event.clip != eventClipFilter)
				{
					continue;
				}

				char label[192];
				sprintf_s(label, "%-10s %6.3f  %s %s", event.clip.c_str(), event.timeSec, event.event.c_str(),
				          event.param.c_str());
				ImGui::PushID(i);
				if (!animator->HasClip(event.clip))
				{
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.4f, 1.f, 1.f)); // clip not registered
					ImGui::Selectable(label, i == selectedEvent);
					ImGui::PopStyleColor();
					if (ImGui::IsItemClicked())
					{
						selectedEvent = i;
					}
				}
				else if (ImGui::Selectable(label, i == selectedEvent))
				{
					selectedEvent = i == selectedEvent ? -1 : i;
				}
				ImGui::PopID();
			}
			ImGui::EndListBox();
		}

		// new events start at the playing clip's current time: pause the tester on the frame you want, then add
		ImGui::BeginDisabled(currClip.empty());
		if (ImGui::Button("+ 現在の時間に追加"))
		{
			desc.events.push_back({currClip, currTime, "HitboxOn", ""});
			selectedEvent = static_cast<int>(desc.events.size()) - 1;
			isChanged = true;
		}
		ImGui::EndDisabled();

		if (selectedEvent >= static_cast<int>(desc.events.size()))
		{
			selectedEvent = -1;
		}
		if (selectedEvent >= 0)
		{
			AnimEventDesc& event = desc.events[selectedEvent];
			ImGui::SeparatorText("選択中のイベント");

			isChanged |= clipCombo("クリップ", event.clip, false);

			const float length = animator->GetClipLength(event.clip);
			isChanged |= ImGui::DragFloat("時間", &event.timeSec, 0.001f, 0.f, length, "%.3f s");
			event.timeSec = std::clamp(event.timeSec, 0.f, length);
			ImGui::BeginDisabled(currClip != event.clip);
			if (ImGui::SmallButton("= 再生中の時間"))
			{
				event.timeSec = currTime;
				isChanged = true;
			}
			ImGui::EndDisabled();

			// event name: type it, or pick one already in use (HitboxOn / HitboxOff are always offered)
			isChanged |= InputString("イベント", event.event);
			ImGui::SameLine();
			if (ImGui::BeginCombo("##eventPick", nullptr, ImGuiComboFlags_NoPreview))
			{
				std::vector<std::string> names = {"HitboxOn", "HitboxOff"};
				for (const AnimEventDesc& other : desc.events)
				{
					if (std::ranges::find(names, other.event) == names.end())
					{
						names.push_back(other.event);
					}
				}
				for (const std::string& name : names)
				{
					if (ImGui::Selectable(name.c_str(), name == event.event))
					{
						event.event = name;
						isChanged = true;
					}
				}
				ImGui::EndCombo();
			}

			// param: for HitboxOn/Off this is a hitbox group, so offer this object's groups
			isChanged |= InputString("パラメータ", event.param);
			if (const auto* volume = object.GetComponent<HitVolumeComponent>())
			{
				ImGui::SameLine();
				if (ImGui::BeginCombo("##paramPick", nullptr, ImGuiComboFlags_NoPreview))
				{
					for (const HitGroup& group : volume->GetGroups())
					{
						if (ImGui::Selectable(group.name.c_str(), group.name == event.param))
						{
							event.param = group.name;
							isChanged = true;
						}
					}
					ImGui::EndCombo();
				}
			}

			if (ImGui::Button("複製##event"))
			{
				AnimEventDesc copy = event; // copy before push_back: it may reallocate
				desc.events.push_back(std::move(copy));
				selectedEvent = static_cast<int>(desc.events.size()) - 1;
				isChanged = true;
			}
			ImGui::SameLine();
			if (ImGui::Button("削除##event"))
			{
				desc.events.erase(desc.events.begin() + selectedEvent);
				selectedEvent = -1;
				isChanged = true;
			}
		}
		ImGui::TreePop();
	}

	if (isChanged)
	{
		animSet.Apply();
	}

	ImGui::PopID();
}
