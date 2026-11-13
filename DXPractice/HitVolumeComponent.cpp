#include "HitVolumeComponent.h"

#include "DebugRenderer.h"
#include "GameContext.h"
#include "GameObject.h"
#include "HitDetectionSystem.h"
#include "SkeletalAnimatorComponent.h"

namespace
{	
	const std::string hitboxOnEvent = "HitboxOn";
	const std::string hitboxOffEvent = "HitboxOff";

	constexpr float axisExtra = 0.2f; // poke axes out of the shape

	// debug
	constexpr DirectX::XMFLOAT4 hurtboxColor{ 0.f, 1.f, 0.f, 1.f };
	constexpr DirectX::XMFLOAT4 hurtboxDisabledColor{ 0.15f, 0.3f, 0.15f, 1.f };
	constexpr DirectX::XMFLOAT4 hitboxActiveColor{ 1.f, 0.15f, 0.15f, 1.f };
	constexpr DirectX::XMFLOAT4 hitboxInactiveColor{ 0.45f, 0.f, 0.f, 1.f };
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

	if (!desc.boneName.empty())
	{
		assert(animator && "bone-attached hit shape needs a SkeletalAnimatorComponent");
		shape.nodeIndex = animator->FindNodeIndex(desc.boneName);
		assert(shape.nodeIndex >= 0 && "hit shape bone not found");
	}

	if (desc.role == HitVolumeRole::HITBOX)
	{
		assert(!desc.groupName.empty() && "every hitbox needs a group");
		shape.groupIndex = FindOrAddGroup(desc.groupName);
	}

	hitShapes.push_back(std::move(shape));
	return static_cast<int>(hitShapes.size() - 1);
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

void HitVolumeComponent::SetHurtboxEnabled(const std::string& name, bool inIsEnabled)
{
	bool isFound = false;
	for (HitShape& shape : hitShapes)
	{
		if (shape.desc.role == HitVolumeRole::HURTBOX && shape.desc.name == name)
		{
			shape.isEnabeld = inIsEnabled;
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
			shape.isEnabeld = inIsEnabled;
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
			for (size_t i = 0; i < hitGroups.size(); i++)
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

		const int index = FindGroup(event.param);
		assert(index >= 0 && "HitboxOn/Off param is not a hitbox group on this object");
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

	for (const HitShape& shape : hitShapes)
	{
		if (!shape.hasWorld)
		{
			continue;
		}

		XMFLOAT4 color;
		if (shape.desc.role == HitVolumeRole::HURTBOX)
		{
			color = shape.isEnabeld ? hurtboxColor : hurtboxDisabledColor;
		}
		else
		{
			color = hitGroups[shape.groupIndex].isActive ? hitboxActiveColor : hitboxInactiveColor;
		}

		debugRenderer.DrawCapsule3D(shape.world.centerA, shape.world.centerB, shape.world.radius, color);

		if (!isShowingBoneAxes || shape.nodeIndex < 0)
		{
			continue;
		}

		// bone axes: X red, Y green, Z blue
		const XMVECTOR rotation = XMLoadFloat4(&shape.boneRot);
		const XMVECTOR origin = XMLoadFloat3(&shape.bonePos);
		const float length = shape.desc.radius + axisExtra;
		const XMVECTOR axes[3] = {
			XMVectorSet(1.f, 0.f, 0.f, 0.f), XMVectorSet(0.f, 1.f, 0.f, 0.f), XMVectorSet(0.f, 0.f, 1.f, 0.f)
		};
		const XMFLOAT4 axisColors[3] = {{1.f, 0.f, 0.f, 1.f}, {0.f, 1.f, 0.f, 1.f}, {0.f, 0.4f, 1.f, 1.f}};

		for (int i = 0; i < 3; i++)
		{
			XMFLOAT3 end;
			XMStoreFloat3(&end, origin + XMVector3Rotate(axes[i], rotation) * length);
			debugRenderer.DrawLine3D(shape.bonePos, end, axisColors[i]);
		}
	}
}

int HitVolumeComponent::FindGroup(const std::string& inGroupName) const
{
	for (size_t i = 0; i < hitGroups.size(); i++)
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
