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

	// hit volumes (groups exist only after their first hitbox is added, so rehits come after)
	auto& hitVolume = object.AddComponent<HitVolumeComponent>();
	for (const HitShapeDesc& shape : data.shapes)
	{
		hitVolume.AddShape(shape);
	}
	for (const GroupRehitDef& rehit : data.rehits)
	{
		hitVolume.SetGroupRehitInterval(rehit.group, rehit.intervalSec);
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
