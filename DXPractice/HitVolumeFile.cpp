#include "HitVolumeFile.h"

#include <nlohmann/json.hpp>
#include <cmath>
#include <filesystem>
#include <fstream>

namespace
{
	constexpr int fileVersion = 1;

	// role <-> string (array order = HitVolumeRole order)
	constexpr const char* roleNames[] = {"hurtbox", "hitbox", "body"};

	const char* RoleToString(HitVolumeRole role)
	{
		return roleNames[static_cast<int>(role)];
	}

	bool StringToRole(const std::string& text, HitVolumeRole& out)
	{
		for (int i = 0; i < 3; i++)
		{
			if (text == roleNames[i])
			{
				out = static_cast<HitVolumeRole>(i);
				return true;
			}
		}
		return false;
	}

	// 3 decimals = 1 mm, keeps the file readable and the git diffs small (same as SceneLayout)
	double Round3(float v)
	{
		return std::round(static_cast<double>(v) * 1000.0) / 1000.0;
	}

	nlohmann::ordered_json Float3ToJson(const DirectX::XMFLOAT3& v)
	{
		return nlohmann::ordered_json::array({Round3(v.x), Round3(v.y), Round3(v.z)});
	}

	bool JsonToFloat3(const nlohmann::ordered_json& json, DirectX::XMFLOAT3& out)
	{
		if (!json.is_array() || json.size() != 3)
		{
			return false;
		}
		for (const auto& element : json)
		{
			if (!element.is_number())
			{
				return false;
			}
		}
		out = {json[0].get<float>(), json[1].get<float>(), json[2].get<float>()};
		return true;
	}

	nlohmann::ordered_json Float4ToJson(const DirectX::XMFLOAT4& v)
	{
		return nlohmann::ordered_json::array({Round3(v.x), Round3(v.y), Round3(v.z), Round3(v.w)});
	}

	bool JsonToFloat4(const nlohmann::ordered_json& json, DirectX::XMFLOAT4& out)
	{
		if (!json.is_array() || json.size() != 4)
		{
			return false;
		}
		for (const auto& element : json)
		{
			if (!element.is_number())
			{
				return false;
			}
		}
		out = {json[0].get<float>(), json[1].get<float>(), json[2].get<float>(), json[3].get<float>()};
		return true;
	}

	std::string GetString(const nlohmann::ordered_json& obj, const char* key)
	{
		const auto it = obj.find(key);
		return it != obj.end() && it->is_string() ? it->get<std::string>() : std::string{};
	}

	float GetFloat(const nlohmann::ordered_json& obj, const char* key, float fallback)
	{
		const auto it = obj.find(key);
		return it != obj.end() && it->is_number() ? it->get<float>() : fallback;
	}
}

HitVolumeFile::Result HitVolumeFile::Load(const std::string& path, HitVolumeDesc& out)
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

	HitVolumeDesc desc;

	if (const auto it = root.find("groups"); it != root.end() && it->is_array())
	{
		for (const auto& entry : *it)
		{
			if (!entry.is_object())
			{
				continue;
			}
			HitGroupDesc group;
			group.name = GetString(entry, "name");
			group.rehitIntervalSec = GetFloat(entry, "rehitIntervalSec", 0.f);
			group.damageScale = GetFloat(entry, "damageScale", group.damageScale);
			group.staggerScale = GetFloat(entry, "staggerScale", group.staggerScale);
			if (const auto color = entry.find("vfxColor"); color != entry.end())
			{
				JsonToFloat4(*color, group.vfxColor); // missing / malformed = no glow
			}
			if (!group.name.empty())
			{
				desc.groups.push_back(std::move(group));
			}
		}
	}

	int skipped = 0;
	if (const auto it = root.find("shapes"); it != root.end() && it->is_array())
	{
		for (const auto& entry : *it)
		{
			HitShapeDesc shape;
			if (!entry.is_object() || !StringToRole(GetString(entry, "role"), shape.role))
			{
				skipped += 1; // unknown role: skip instead of guessing
				continue;
			}

			shape.name = GetString(entry, "name");
			shape.groupName = GetString(entry, "group");
			shape.boneName = GetString(entry, "bone");
			if (const auto a = entry.find("a"); a != entry.end())
			{
				JsonToFloat3(*a, shape.localCenterA);
			}
			if (const auto b = entry.find("b"); b != entry.end())
			{
				JsonToFloat3(*b, shape.localCenterB);
			}
			shape.radius = GetFloat(entry, "radius", shape.radius);
			if (const auto s = entry.find("sweeping"); s != entry.end() && s->is_boolean())
			{
				shape.isSweeping = s->get<bool>();
			}
			desc.shapes.push_back(std::move(shape));
		}
	}

	out = std::move(desc);
	result.ok = true;
	result.message = "読み込み：" + std::to_string(out.shapes.size()) + " 個";
	if (skipped > 0)
	{
		result.message += "（" + std::to_string(skipped) + " 個スキップ：不明な role）";
	}
	return result;
}

HitVolumeFile::Result HitVolumeFile::Save(const std::string& path, const HitVolumeDesc& desc)
{
	Result result;

	nlohmann::ordered_json groups = nlohmann::ordered_json::array();
	for (const HitGroupDesc& group : desc.groups)
	{
		nlohmann::ordered_json entry;
		entry["name"] = group.name;
		entry["rehitIntervalSec"] = Round3(group.rehitIntervalSec);
		entry["damageScale"] = Round3(group.damageScale);
		entry["staggerScale"] = Round3(group.staggerScale);
		if (group.vfxColor.w > 0.f)
		{
			entry["vfxColor"] = Float4ToJson(group.vfxColor);
		}
		groups.push_back(std::move(entry));
	}

	nlohmann::ordered_json shapes = nlohmann::ordered_json::array();
	for (const HitShapeDesc& shape : desc.shapes)
	{
		nlohmann::ordered_json entry;
		entry["role"] = RoleToString(shape.role);
		entry["name"] = shape.name;
		if (shape.role == HitVolumeRole::HITBOX)
		{
			entry["group"] = shape.groupName;
		}
		entry["bone"] = shape.boneName;
		entry["a"] = Float3ToJson(shape.localCenterA);
		entry["b"] = Float3ToJson(shape.localCenterB);
		entry["radius"] = Round3(shape.radius);
		if (shape.role == HitVolumeRole::HITBOX)
		{
			entry["sweeping"] = shape.isSweeping;
		}
		shapes.push_back(std::move(entry));
	}

	// one entry per line: readable, and a changed shape is a one-line git diff
	const auto writeArray = [](std::string& text, const nlohmann::ordered_json& array)
	{
		text += "[\n";
		for (size_t i = 0; i < array.size(); i++)
		{
			text += "  " + array[i].dump();
			text += i + 1 < array.size() ? ",\n" : "\n";
		}
		text += " ]";
	};

	std::string text = "{\n";
	text += " \"version\": " + std::to_string(fileVersion) + ",\n";
	text += " \"groups\": ";
	writeArray(text, groups);
	text += ",\n \"shapes\": ";
	writeArray(text, shapes);
	text += "\n}\n";

	// write to .tmp then rename, so a failed write never leaves a half file (same as SceneLayout)
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
	result.message = "保存しました：" + std::to_string(desc.shapes.size()) + " 個 → " + path;
	return result;
}
