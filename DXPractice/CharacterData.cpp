#include "CharacterData.h"

#include <cassert>

#include "AnimationClipCache.h"
#include "GameObject.h"
#include "SkeletalAnimatorComponent.h"

void ApplyCharacterData(GameObject& object, const CharacterData& data)
{
	object.GetTransform()->SetScale(data.scale);

	auto* animator = object.GetComponent<SkeletalAnimatorComponent>();
	assert(animator && "character data needs a SkeletalAnimatorComponent");

	// hit volumes: the JSON file wins; the code tables are the fallback until it exists
	HitVolumeDesc fallback;
	fallback.shapes = data.shapes;
	for (const GroupRehitDef& rehit : data.rehits)
	{
		fallback.groups.push_back({ rehit.group, rehit.intervalSec });
	}

	auto& hitVolume = object.AddComponent<HitVolumeComponent>();
	if (data.hitVolumePath.empty())
	{
		hitVolume.Build(fallback);
	}
	else
	{
		hitVolume.LoadFile(data.hitVolumePath, fallback);
	}

	// animations (events need their clip registered first)
	for (const ClipDef& clip : data.clips)
	{
		animator->AddClip(
			AnimationClipCache::LoadClip(data.animDir + clip.name + ".fbx", animator->GetSkeleton()),
			clip.loop);
	}
	animator->SetFadeTable(data.fadeTable);

	for (const EventDef& event : data.events)
	{
		animator->AddEvent(event.clip, event.timeSec, event.event, event.param);
	}

	animator->PlayClip(data.initialClip);
}