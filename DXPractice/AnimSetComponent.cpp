#include "AnimSetComponent.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <filesystem>
#include <memory>

#include "AnimationClipCache.h"
#include "AnimationFadeTable.h"
#include "GameObject.h"
#include "SkeletalAnimatorComponent.h"
#include "Win.h"

void AnimSetComponent::OnAttach(GameContext& context)
{
	IComponent::OnAttach(context);

	animator = owner->GetComponent<SkeletalAnimatorComponent>();
	assert(animator && "AnimSetComponent needs a SkeletalAnimatorComponent on the object first");
}

void AnimSetComponent::LoadFile(const std::string& path, const AnimSetDesc& fallback)
{
	dataPath = path;

	AnimSetDesc loaded;
	const AnimSetFile::Result result = AnimSetFile::Load(path, loaded);
	if (result.ok)
	{
		desc = std::move(loaded);
		loadMessage = result.message;
	}
	else
	{
		desc = fallback;
		loadMessage = result.fileFound
			? result.message + "（コードの定義を使用）"
			: "ファイルなし：コードの定義を使用（保存で作成）";
		OutputDebugStringA(("[AnimSet] " + path + ": " + loadMessage + "\n").c_str());
	}

	if (!animator)
	{
		return;
	}

	RegisterClips();
	Apply();
	animator->PlayClip(desc.initialClip);
}

bool AnimSetComponent::ReloadFile(std::string& outMessage)
{
	AnimSetDesc loaded;
	const AnimSetFile::Result result = AnimSetFile::Load(dataPath, loaded);
	outMessage = result.message;
	if (!result.ok)
	{
		return false;
	}

	desc = std::move(loaded);
	RegisterClips(); // clips added in the file since startup
	Apply();
	return true;
}

bool AnimSetComponent::SaveFile(std::string& outMessage) const
{
	if (dataPath.empty())
	{
		outMessage = "保存先なし（LoadFile で登録されていません）";
		return false;
	}

	const AnimSetFile::Result result = AnimSetFile::Save(dataPath, desc);
	outMessage = result.message;
	return result.ok;
}

void AnimSetComponent::RegisterClips()
{
	for (const AnimClipDesc& clip : desc.clips)
	{
		if (animator->HasClip(clip.name))
		{
			continue;
		}

		// a clip in the file whose FBX is gone must not assert inside the loader: skip it
		const std::string path = desc.animDir + clip.name + ".fbx";
		std::error_code errCode;
		if (!std::filesystem::exists(path, errCode))
		{
			OutputDebugStringA(("[AnimSet] clip file not found, skipped: " + path + "\n").c_str());
			continue;
		}

		animator->AddClip(AnimationClipCache::LoadClip(path, animator->GetSkeleton()), clip.isLoop);
	}
}

void AnimSetComponent::Apply()
{
	if (!animator)
	{
		return;
	}

	// loop flags (take effect the next time the clip is played)
	for (const AnimClipDesc& clip : desc.clips)
	{
		animator->SetClipLoop(clip.name, clip.isLoop);
	}

	// fade table: a fresh table per apply. It is tiny, and other characters keep their own.
	auto fadeTable = std::make_shared<AnimationFadeTable>(desc.defaultFadeSec);
	for (const AnimFadeDesc& fade : desc.fades)
	{
		fadeTable->SetFade(fade.from, fade.to, fade.sec);
	}
	animator->SetFadeTable(std::move(fadeTable));

	// events: rebuild all. Invalid ones (clip not registered) are skipped here instead of asserting,
	// because they can come from a hand-edited file; times are clamped to the clip.
	animator->ClearEvents();
	for (const AnimEventDesc& event : desc.events)
	{
		if (!animator->HasClip(event.clip))
		{
			continue;
		}
		const float timeSec = std::clamp(event.timeSec, 0.f, animator->GetClipLength(event.clip));
		animator->AddEvent(event.clip, timeSec, event.event, event.param);
	}
}

int AnimSetComponent::AddClipsFromFolder()
{
	std::error_code errCode;
	if (desc.animDir.empty() || !std::filesystem::is_directory(desc.animDir, errCode))
	{
		return 0;
	}

	int added = 0;
	for (const auto& entry : std::filesystem::directory_iterator(desc.animDir, errCode))
	{
		std::string extension = entry.path().extension().string();
		std::ranges::transform(extension, extension.begin(), [](unsigned char c) { return std::tolower(c); });
		if (!entry.is_regular_file() || extension != ".fbx")
		{
			continue;
		}

		const std::string name = entry.path().stem().string();
		if (animator->HasClip(name))
		{
			continue;
		}

		// same path string as RegisterClips, so the clip cache sees one key per file
		desc.clips.push_back({ name, false });
		animator->AddClip(AnimationClipCache::LoadClip(desc.animDir + name + ".fbx", animator->GetSkeleton()), false);
		added += 1;
	}

	std::ranges::sort(desc.clips, {}, &AnimClipDesc::name);
	return added;
}