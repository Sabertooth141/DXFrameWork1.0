#include "CharacterData.h"

#include <cassert>

#include "ActionController.h"
#include "AnimSetComponent.h"
#include "GameObject.h"
#include "SkeletalAnimatorComponent.h"

void ApplyCharacterData(GameObject& object, const CharacterData& data)
{
	object.GetTransform()->SetScale(data.scale);

	auto* animator = object.GetComponent<SkeletalAnimatorComponent>();
	assert(animator && "character data needs a SkeletalAnimatorComponent");
	const DirectX::XMFLOAT3& rotationDeg = data.modelRotationDeg;
	animator->SetModelRotation({
		DirectX::XMConvertToRadians(rotationDeg.x),
		DirectX::XMConvertToRadians(rotationDeg.y),
		DirectX::XMConvertToRadians(rotationDeg.z)
	});

	// hit volumes: the JSON file wins; the code tables are the fallback until it exists
	HitVolumeDesc fallback;
	fallback.shapes = data.shapes;
	for (const GroupRehitDef& rehit : data.rehits)
	{
		fallback.groups.push_back({rehit.group, rehit.intervalSec});
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

	// animations: the JSON file wins; the code tables are the fallback until it exists
	AnimSetDesc animFallback;
	animFallback.animDir = data.animDir;
	animFallback.initialClip = data.initialClip;
	animFallback.defaultFadeSec = data.defaultFadeSec;
	for (const ClipDef& clip : data.clips)
	{
		animFallback.clips.push_back({clip.name, clip.loop});
	}
	for (const FadeDef& fade : data.fades)
	{
		animFallback.fades.push_back({fade.from, fade.to, fade.sec});
	}
	for (const EventDef& event : data.events)
	{
		animFallback.events.push_back({event.clip, event.timeSec, event.event, event.param});
	}

	object.AddComponent<AnimSetComponent>().LoadFile(data.animSetPath, animFallback);

	if (!data.rootMotionNode.empty())
	{
		animator->SetRootMotionNode(data.rootMotionNode);
		for (const RootMotionDef& def : data.rootMotionClips)
		{
			animator->SetClipRootMotion(def.clip, def.mode);
		}

		if (!data.rootMotionAnchorClip.empty())
		{
			animator->SetRootMotionAnchor(data.rootMotionAnchorClip);
		}
	}

	object.AddComponent<ActionController>();
}
