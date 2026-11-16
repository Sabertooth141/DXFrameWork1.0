#pragma once
#include <string>

#include "AnimSetFile.h"
#include "IComponent.h"

class SkeletalAnimatorComponent;

// Owns the editable animation data of one character (clips, fades, events) and pushes it
// into the SkeletalAnimatorComponent. The animator stays a pure player; this is the
// "what to play and when things happen" layer that the editor edits and the JSON file stores.
class AnimSetComponent : public IComponent
{
public:
	void OnAttach(GameContext& context) override;

	// loads path if it exists, otherwise uses fallback (code-defined data); the path is remembered
	// either way so the editor's Save creates the file. Then registers the clips and plays initialClip.
	void LoadFile(const std::string& path, const AnimSetDesc& fallback);
	bool ReloadFile(std::string& outMessage); // editor "Revert"
	bool SaveFile(std::string& outMessage) const;

	// editor: push fades / events / loop flags to the animator after desc was edited.
	// Clips are never removed at runtime (playback may hold them); adding goes through AddClipsFromFolder.
	void Apply();

	// registers every .fbx in animDir that is not in the clip list yet (loop = false). Returns the count.
	int AddClipsFromFolder();

	AnimSetDesc& GetDesc()
	{
		return desc;
	}

	const std::string& GetDataPath() const
	{
		return dataPath;
	}

	const std::string& GetLoadMessage() const
	{
		return loadMessage;
	}

private:
	void RegisterClips();

private:
	SkeletalAnimatorComponent* animator = nullptr;
	AnimSetDesc desc;
	std::string dataPath;
	std::string loadMessage;
};