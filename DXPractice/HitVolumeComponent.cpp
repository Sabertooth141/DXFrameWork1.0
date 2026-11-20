#include "HitVolumeComponent.h"

#include <algorithm>

#include "DebugRenderer.h"
#include "GameContext.h"
#include "GameObject.h"
#include "HitDetectionSystem.h"
#include "HitVolumeFile.h"
#include "SkeletalAnimatorComponent.h"
#include "VfxRenderer.h"

namespace
{
	const std::string hitboxOnEvent = "HitboxOn";
	const std::string hitboxOffEvent = "HitboxOff";

	constexpr float axisExtra = 0.2f; // poke axes out of the shape

	// debug
	constexpr DirectX::XMFLOAT4 hurtboxColor{0.f, 1.f, 0.f, 1.f};
	constexpr DirectX::XMFLOAT4 hurtboxDisabledColor{0.15f, 0.3f, 0.15f, 1.f};
	constexpr DirectX::XMFLOAT4 hitboxActiveColor{1.f, 0.15f, 0.15f, 1.f};
	constexpr DirectX::XMFLOAT4 hitboxInactiveColor{0.45f, 0.f, 0.f, 1.f};
	constexpr DirectX::XMFLOAT4 bodyColor{0.2f, 0.6f, 1.f, 1.f};
	constexpr DirectX::XMFLOAT4 highlightColor{1.f, 0.9f, 0.1f, 1.f};
	constexpr DirectX::XMFLOAT4 boneMissingColor{1.f, 0.f, 1.f, 1.f};

	const std::string defaultGroupName = "default";
}

void HitVolumeComponent::OnAttach(GameContext& context)
{
	IComponent::OnAttach(context);

	animator = owner->GetComponent<SkeletalAnimatorComponent>();
	context.hitDetectionSys.Register(this);
}

int HitVolumeComponent::AddShape(const HitShapeDesc& desc)
{
	HitShape shape;
	shape.desc = desc;

	// shapes now come from an editable file, so a wrong bone name must not crash:
	// the shape follows the root instead and is drawn magenta
	if (!desc.boneName.empty())
	{
		if (animator)
		{
			shape.nodeIndex = animator->FindNodeIndex(desc.boneName);
		}
		shape.isBoneMissing = shape.nodeIndex < 0;
	}

	if (desc.role == HitVolumeRole::HITBOX)
	{
		if (shape.desc.groupName.empty())
		{
			shape.desc.groupName = defaultGroupName; // every hitbox needs a group
		}
		shape.groupIndex = FindOrAddGroup(shape.desc.groupName);
	}

	hitShapes.push_back(std::move(shape));
	return static_cast<int>(hitShapes.size() - 1);
}

void HitVolumeComponent::Build(const HitVolumeDesc& desc)
{
	hitShapes.clear();
	hitGroups.clear();

	// groups first so their rehit intervals are set before hitboxes reference them
	for (const HitGroupDesc& groupDesc : desc.groups)
	{
		HitGroup& hitGroup = hitGroups[FindOrAddGroup(groupDesc.name)];
		hitGroup.rehitIntervalSec = groupDesc.rehitIntervalSec;
		hitGroup.damageScale = groupDesc.damageScale;
		hitGroup.staggerScale = groupDesc.staggerScale;
		hitGroup.vfxColor = groupDesc.vfxColor;
	}
	for (const HitShapeDesc& shape : desc.shapes)
	{
		AddShape(shape);
	}
}

HitVolumeDesc HitVolumeComponent::ExportDesc() const
{
	HitVolumeDesc desc;
	desc.shapes.reserve(hitShapes.size());
	for (const HitShape& shape : hitShapes)
	{
		desc.shapes.push_back(shape.desc);
	}

	// only groups that still have a hitbox, so deleted groups do not pile up in the file
	for (size_t i = 0; i < hitGroups.size(); i++)
	{
		const bool isUsed = std::any_of(hitShapes.begin(), hitShapes.end(), [i](const HitShape& shape)
		{
			return shape.groupIndex == static_cast<int>(i);
		});
		if (isUsed)
		{
			desc.groups.push_back({
				hitGroups[i].name,
				hitGroups[i].rehitIntervalSec,
				hitGroups[i].damageScale,
				hitGroups[i].staggerScale,
				hitGroups[i].vfxColor
			});
		}
	}
	return desc;
}

void HitVolumeComponent::LoadFile(const std::string& path, const HitVolumeDesc& fallback)
{
	dataPath = path;

	HitVolumeDesc loaded;
	const HitVolumeFile::Result result = HitVolumeFile::Load(path, loaded);
	if (result.ok)
	{
		Build(loaded);
		loadMessage = result.message;
		return;
	}

	Build(fallback);
	loadMessage = result.fileFound
		              ? result.message + "（コードの定義を使用）"
		              : "ファイルなし：コードの定義を使用（保存で作成）";
	OutputDebugStringA(("[HitVolume] " + path + ": " + loadMessage + "\n").c_str());
}

bool HitVolumeComponent::ReloadFile(std::string& outMessage)
{
	HitVolumeDesc loaded;
	const HitVolumeFile::Result result = HitVolumeFile::Load(dataPath, loaded);
	outMessage = result.message;
	if (result.ok)
	{
		Build(loaded);
	}
	return result.ok;
}

bool HitVolumeComponent::SaveFile(std::string& outMessage) const
{
	if (dataPath.empty())
	{
		outMessage = "保存先なし（LoadFile で登録されていません）";
		return false;
	}

	const HitVolumeFile::Result result = HitVolumeFile::Save(dataPath, ExportDesc());
	outMessage = result.message;
	return result.ok;
}

void HitVolumeComponent::SetGroupRehitInterval(const std::string& groupName, float intervalSec)
{
	const int index = FindGroup(groupName);
	assert(index >= 0 && "unknown hitbox group");
	if (index >= 0)
	{
		hitGroups[index].rehitIntervalSec = intervalSec;
	}
}

void HitVolumeComponent::SetGroupVFXColor(const std::string& groupName, const DirectX::XMFLOAT4& inColor)
{
	const int index = FindGroup(groupName);
	assert(index >= 0 && "unknown hitbox group");
	if (index >= 0)
	{
		hitGroups[index].vfxColor = inColor;
	}
}

void HitVolumeComponent::SetHurtboxEnabled(const std::string& name, bool inIsEnabled)
{
	bool isFound = false;
	for (HitShape& shape : hitShapes)
	{
		if (shape.desc.role == HitVolumeRole::HURTBOX && shape.desc.name == name)
		{
			shape.isEnabled = inIsEnabled;
			isFound = true;
		}
	}
	assert(isFound && "no hurtbox with that name");
}

void HitVolumeComponent::SetAllHurtboxEnabled(bool inIsEnabled)
{
	for (HitShape& shape : hitShapes)
	{
		if (shape.desc.role == HitVolumeRole::HURTBOX)
		{
			shape.isEnabled = inIsEnabled;
		}
	}
}

void HitVolumeComponent::ApplyAnimationEvents()
{
	if (!animator)
	{
		return;
	}

	for (const FiredAnimationEvent& event : animator->GetFiredEvents())
	{
		if (event.type == FiredEventType::INTERRUPTED)
		{
			for (int i = 0; i < static_cast<int>(hitGroups.size()); i++)
			{
				if (hitGroups[i].isActive && hitGroups[i].activatedByClip == event.clipName)
				{
					ActivateGroup(i, false, {});
				}
			}

			continue;
		}

		const bool isOn = event.eventName == hitboxOnEvent;
		// if neither on nor off
		if (!isOn && event.eventName != hitboxOffEvent)
		{
			continue;
		}

		// a group can disappear while editing (last hitbox deleted), so skip instead of asserting
		const int index = FindGroup(event.param);
		if (index >= 0)
		{
			ActivateGroup(index, isOn, event.clipName);
		}
	}
}

bool HitVolumeComponent::CanHit(int groupIndex, const GameObject* victim, float currTime) const
{
	const HitGroup& group = hitGroups[groupIndex];
	for (const HitRecord& record : group.hitRecords)
	{
		if (record.victim != victim)
		{
			continue;
		}
		if (group.rehitIntervalSec <= 0.f)
		{
			return false;
		}
		return currTime - record.lastHitTimeSec >= group.rehitIntervalSec;
	}
	return true;
}

void HitVolumeComponent::RecordHit(int groupIndex, const GameObject* victim, float currTime)
{
	for (HitRecord& record : hitGroups[groupIndex].hitRecords)
	{
		if (record.victim == victim)
		{
			record.lastHitTimeSec = currTime;
			return;
		}
	}

	hitGroups[groupIndex].hitRecords.push_back({victim, currTime});
}

void HitVolumeComponent::UpdateWorldShapes()
{
	using namespace DirectX;

	const XMMATRIX rootWorldMatrix = owner->GetTransform()->GetMatrix();

	for (HitShape& shape : hitShapes)
	{
		XMMATRIX boneWorldMatrix;
		if (shape.nodeIndex >= 0)
		{
			boneWorldMatrix = animator->GetNodeWorldMatrix(shape.nodeIndex);
		}
		else
		{
			boneWorldMatrix = rootWorldMatrix;
		}

		XMVECTOR scale, rotation, position;

		XMMatrixDecompose(&scale, &rotation, &position, boneWorldMatrix);

		shape.prevWorld = shape.world;
		shape.hasPrev = shape.hasWorld;

		const XMVECTOR centerA = position + XMVector3Rotate(XMLoadFloat3(&shape.desc.localCenterA), rotation);
		const XMVECTOR centerB = position + XMVector3Rotate(XMLoadFloat3(&shape.desc.localCenterB), rotation);
		XMStoreFloat3(&shape.world.centerA, centerA);
		XMStoreFloat3(&shape.world.centerB, centerB);
		shape.world.radius = shape.desc.radius;
		shape.hasWorld = true;

		XMStoreFloat3(&shape.bonePos, position);
		XMStoreFloat4(&shape.boneRot, rotation);
	}
}

void HitVolumeComponent::DrawDebug(DebugRenderer& debugRenderer) const
{
	using namespace DirectX;

	for (size_t i = 0; i < hitShapes.size(); i++)
	{
		const HitShape& shape = hitShapes[i];
		if (!shape.hasWorld)
		{
			continue;
		}

		const bool isHighlighted = static_cast<int>(i) == highlightedShape;

		XMFLOAT4 color;
		if (isHighlighted)
		{
			color = highlightColor;
		}
		else if (shape.isBoneMissing)
		{
			color = boneMissingColor;
		}
		else if (shape.desc.role == HitVolumeRole::HURTBOX)
		{
			color = shape.isEnabled ? hurtboxColor : hurtboxDisabledColor;
		}
		else if (shape.desc.role == HitVolumeRole::BODY)
		{
			color = bodyColor;
		}
		else
		{
			color = hitGroups[shape.groupIndex].isActive ? hitboxActiveColor : hitboxInactiveColor;
		}

		debugRenderer.DrawCapsule3D(shape.world.centerA, shape.world.centerB, shape.world.radius, color);
	}
}

void HitVolumeComponent::DrawVFX(VfxRenderer& vfxRenderer) const
{
	for (const HitShape& shape : hitShapes)
	{
		if (shape.desc.role != HitVolumeRole::HITBOX || !shape.hasWorld)
		{
			continue;
		}

		const HitGroup& group = hitGroups[shape.groupIndex];
		if (!group.isActive || group.vfxColor.w <= 0.f)
		{
			continue;
		}

		vfxRenderer.DrawCapsule(shape.world.centerA, shape.world.centerB, shape.world.radius, group.vfxColor);
	}
}

int HitVolumeComponent::FindGroup(const std::string& inGroupName) const
{
	for (int i = 0; i < static_cast<int>(hitGroups.size()); i++)
	{
		if (hitGroups[i].name == inGroupName)
		{
			return i;
		}
	}

	return -1;
}

int HitVolumeComponent::FindOrAddGroup(const std::string& inGroupName)
{
	const int index = FindGroup(inGroupName);
	if (index >= 0)
	{
		return index;
	}

	hitGroups.push_back({.name = inGroupName});
	return static_cast<int>(hitGroups.size() - 1);
}

void HitVolumeComponent::SetGroupActive(const std::string& groupName, bool inIsActive)
{
	const int index = FindGroup(groupName);
	assert(index >= 0 && "unknown hitbox group");

	if (index >= 0)
	{
		ActivateGroup(index, inIsActive, {});
	}
}

bool HitVolumeComponent::IsGroupActive(const std::string& groupName)
{
	const int index = FindGroup(groupName);
	return index >= 0 && hitGroups[index].isActive;
}

void HitVolumeComponent::ActivateGroup(int groupIndex, bool isActive, const std::string& clipName)
{
	HitGroup& group = hitGroups[groupIndex];
	if (group.isActive == isActive)
	{
		return;
	}

	group.isActive = isActive;
	if (!isActive)
	{
		group.activatedByClip.clear();
		return;
	}

	group.activationCount += 1;
	group.activatedByClip = clipName;
	group.hitRecords.clear();

	for (HitShape& shape : hitShapes)
	{
		if (shape.groupIndex == groupIndex)
		{
			shape.hasWorld = false;
		}
	}
}
