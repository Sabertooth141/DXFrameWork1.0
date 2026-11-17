#pragma once
// Small helpers shared by the editor panels (EditorUI, EditorViewport, component inspectors)
#include <imgui.h>
#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

#include "GameObject.h"

namespace EditorUtils
{
	inline bool InputString(const char* label, std::string& value)
	{
		char buffer[128];
		strncpy_s(buffer, value.c_str(), _TRUNCATE);
		if (ImGui::InputText(label, buffer, sizeof(buffer)))
		{
			value = buffer;
			return true;
		}
		return false;
	}

	// case-insensitive substring match for the bone filter ("wing" finds "Wings_l1")
	inline bool ContainsNoCase(const std::string& text, const char* filter)
	{
		if (!filter || !filter[0])
		{
			return true;
		}
		const std::string_view needle(filter);
		const auto it = std::search(text.begin(), text.end(), needle.begin(), needle.end(), [](char a, char b)
		{
			return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
		});
		return it != text.end();
	}

	// green when ok, red otherwise (file save / load result line)
	inline void StatusText(const std::string& message, bool isOk)
	{
		if (message.empty())
		{
			return;
		}
		const ImVec4 color = isOk ? ImVec4(0.6f, 0.9f, 0.6f, 1.f) : ImVec4(1.f, 0.5f, 0.4f, 1.f);
		ImGui::TextColored(color, "%s", message.c_str());
	}

	inline bool Is2DObject(GameObject& obj)
	{
		return obj.GetComponent<AnimatorComponent>() != nullptr;
	}

	inline bool IsObjectScreenSpace(GameObject& obj)
	{
		auto* anim = obj.GetComponent<AnimatorComponent>();
		return anim && anim->IsScreenSpaceEnabled();
	}

	// エディタから動かしたらスリープ中の剛体を起こす
	inline void WakeObjRb(GameObject& obj, bool stop)
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
}
