#include "AnimSetFile.h"

#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>

namespace
{
	constexpr int fileVersion = 1;

	// 3 decimals = 1 ms, readable and stable in git diffs (same as SceneLayout / HitVolumeFile)
	double Round3(float v)
	{
		return std::round(static_cast<double>(v) * 1000.0) / 1000.0;
	}

	std::string GetString(const nlohmann::ordered_json& obj, const char* key, const std::string& fallback = {})
	{
		const auto it = obj.find(key);
		return it != obj.end() && it->is_string() ? it->get<std::string>() : fallback;
	}

	float GetFloat(const nlohmann::ordered_json& obj, const char* key, float fallback)
	{
		const auto it = obj.find(key);
		return it != obj.end() && it->is_number() ? it->get<float>() : fallback;
	}

	bool GetBool(const nlohmann::ordered_json& obj, const char* key, bool fallback)
	{
		const auto it = obj.find(key);
		return it != obj.end() && it->is_boolean() ? it->get<bool>() : fallback;
	}

	// calls fn for every object element of root[key]
	template <typename Fn>
	void ForEachObject(const nlohmann::ordered_json& root, const char* key, Fn&& fn)
	{
		const auto it = root.find(key);
		if (it == root.end() || !it->is_array())
		{
			return;
		}
		for (const auto& entry : *it)
		{
			if (entry.is_object())
			{
				fn(entry);
			}
		}
	}

	// one entry per line: readable, and one changed event = one-line git diff
	void WriteArray(std::string& text, const char* key, const nlohmann::ordered_json& array, bool isLast)
	{
		text += " \"" + std::string(key) + "\": [\n";
		for (size_t i = 0; i < array.size(); i++)
		{
			text += "  " + array[i].dump();
			text += i + 1 < array.size() ? ",\n" : "\n";
		}
		text += isLast ? " ]\n" : " ],\n";
	}
}

AnimSetFile::Result AnimSetFile::Load(const std::string& path, AnimSetDesc& out)
{
	Result result;

	std::error_code errCode;
	if (!std::filesystem::exists(path, errCode))
	{
		result.message = "ファイルなし：" + path;
		return result;
	}
	result.fileFound = true;

	std::ifstream in(path, std::ios::binary);
	const nlohmann::ordered_json root = nlohmann::ordered_json::parse(in, nullptr, false, true);
	if (root.is_discarded() || !root.is_object())
	{
		result.message = "読み込み失敗：JSON の形式が不正です（" + path + "）";
		return result;
	}

	if (const auto it = root.find("version"); it != root.end() && it->is_number_integer()
		&& it->get<int>() > fileVersion)
	{
		result.message = "読み込み失敗：このエディタより新しい形式です（" + path + "）";
		return result;
	}

	AnimSetDesc desc;
	desc.animDir = GetString(root, "animDir");
	desc.initialClip = GetString(root, "initialClip");
	desc.defaultFadeSec = GetFloat(root, "defaultFadeSec", desc.defaultFadeSec);

	ForEachObject(root, "clips", [&](const auto& entry)
		{
			AnimClipDesc clip{ GetString(entry, "name"), GetBool(entry, "loop", false) };
			if (!clip.name.empty())
			{
				desc.clips.push_back(std::move(clip));
			}
		});

	ForEachObject(root, "fades", [&](const auto& entry)
		{
			desc.fades.push_back({
				GetString(entry, "from", "*"), GetString(entry, "to", "*"), GetFloat(entry, "sec", desc.defaultFadeSec)
				});
		});

	ForEachObject(root, "events", [&](const auto& entry)
		{
			AnimEventDesc event{
				GetString(entry, "clip"), GetFloat(entry, "time", 0.f), GetString(entry, "event"), GetString(entry, "param")
			};
			if (!event.clip.empty() && !event.event.empty())
			{
				desc.events.push_back(std::move(event));
			}
		});

	out = std::move(desc);
	result.ok = true;
	result.message = "読み込み：クリップ " + std::to_string(out.clips.size()) + "、フェード " +
		std::to_string(out.fades.size()) + "、イベント " + std::to_string(out.events.size());
	return result;
}

AnimSetFile::Result AnimSetFile::Save(const std::string& path, const AnimSetDesc& desc)
{
	Result result;

	nlohmann::ordered_json clips = nlohmann::ordered_json::array();
	for (const AnimClipDesc& clip : desc.clips)
	{
		nlohmann::ordered_json entry;
		entry["name"] = clip.name;
		entry["loop"] = clip.isLoop;
		clips.push_back(std::move(entry));
	}

	nlohmann::ordered_json fades = nlohmann::ordered_json::array();
	for (const AnimFadeDesc& fade : desc.fades)
	{
		nlohmann::ordered_json entry;
		entry["from"] = fade.from;
		entry["to"] = fade.to;
		entry["sec"] = Round3(fade.sec);
		fades.push_back(std::move(entry));
	}

	// events are saved sorted by clip, then time: the file reads like a timeline per clip,
	// and the order you added them in the editor does not create diffs
	std::vector<AnimEventDesc> sortedEvents = desc.events;
	std::ranges::stable_sort(sortedEvents, [](const AnimEventDesc& a, const AnimEventDesc& b)
		{
			return a.clip != b.clip ? a.clip < b.clip : a.timeSec < b.timeSec;
		});

	nlohmann::ordered_json events = nlohmann::ordered_json::array();
	for (const AnimEventDesc& event : sortedEvents)
	{
		nlohmann::ordered_json entry;
		entry["clip"] = event.clip;
		entry["time"] = Round3(event.timeSec);
		entry["event"] = event.event;
		if (!event.param.empty())
		{
			entry["param"] = event.param;
		}
		events.push_back(std::move(entry));
	}

	std::string text = "{\n";
	text += " \"version\": " + std::to_string(fileVersion) + ",\n";
	text += " \"animDir\": " + nlohmann::ordered_json(desc.animDir).dump() + ",\n";
	text += " \"initialClip\": " + nlohmann::ordered_json(desc.initialClip).dump() + ",\n";
	text += " \"defaultFadeSec\": " + nlohmann::ordered_json(Round3(desc.defaultFadeSec)).dump() + ",\n";
	WriteArray(text, "clips", clips, false);
	WriteArray(text, "fades", fades, false);
	WriteArray(text, "events", events, true);
	text += "}\n";

	// write to .tmp then rename, so a failed write never leaves a half file
	const std::filesystem::path filePath = path;
	std::error_code errCode;
	std::filesystem::create_directories(filePath.parent_path(), errCode);

	std::filesystem::path tempPath = filePath;
	tempPath += ".tmp";
	{
		std::ofstream out(tempPath, std::ios::binary | std::ios::trunc);
		if (!out)
		{
			result.message = "保存失敗：" + path + " を開けません";
			return result;
		}
		out << text;
		if (!out)
		{
			result.message = "保存失敗：書き込みエラー";
			return result;
		}
	}
	std::filesystem::rename(tempPath, filePath, errCode);
	if (errCode)
	{
		std::filesystem::remove(tempPath, errCode);
		result.message = "保存失敗：" + path + " を置き換えられません";
		return result;
	}

	result.ok = true;
	result.fileFound = true;
	result.message = "保存しました → " + path;
	return result;
}