#include "HitDetectionSystem.h"

#include "GameObject.h"

namespace
{
	constexpr int sweepSubSteps = 4;

	Capsule3D LerpCapsule(const Capsule3D& startCapsule, const Capsule3D& endCapsule, float t)
	{
		using namespace DirectX;

		Capsule3D resultCapsule;
		XMStoreFloat3(&resultCapsule.centerA,
		              XMVectorLerp(XMLoadFloat3(&startCapsule.centerA), XMLoadFloat3(&endCapsule.centerA), t));
		XMStoreFloat3(&resultCapsule.centerB,
		              XMVectorLerp(XMLoadFloat3(&startCapsule.centerB), XMLoadFloat3(&endCapsule.centerB), t));
		resultCapsule.radius = endCapsule.radius;

		return resultCapsule;
	}

	CapsuleContact3D TestHitbox(const HitShape& hitbox, const Capsule3D& hurtbox)
	{
		if (!hitbox.desc.isSweeping || !hitbox.hasPrev)
		{
			return TestCapsuleVsCapsule(hitbox.world, hurtbox);
		}

		for (int step = 1; step <= sweepSubSteps; step++)
		{
			const float t = static_cast<float>(step) / sweepSubSteps;
			const CapsuleContact3D contact =
				TestCapsuleVsCapsule(LerpCapsule(hitbox.prevWorld, hitbox.world, t), hurtbox);
			if (contact.isOverlap)
			{
				return contact;
			}
		}

		return {};
	}
}

void HitDetectionSystem::Update(float deltaTime)
{
	timeSec += deltaTime;

	for (HitVolumeComponent* volume : volumes)
	{
		volume->ApplyAnimationEvents();
		volume->UpdateWorldShapes();
	}

	std::vector<HitEvent> hitEvents;
	for (HitVolumeComponent* attacker : volumes)
	{
		for (HitVolumeComponent* victim : volumes)
		{
			if (attacker->owner != victim->owner)
			{
				DetectPair(*attacker, *victim, hitEvents);
			}
		}
	}

	for (const HitEvent& event : hitEvents)
	{
		event.attacker->NotifyHitDealt(event);
		event.victim->NotifyHitTaken(event);
	}
}

void HitDetectionSystem::DetectPair(HitVolumeComponent& attacker, HitVolumeComponent& victim, std::vector<HitEvent>& outHits) const
{
	const std::vector<HitGroup>& groups = attacker.GetGroups();
	const std::vector<HitShape>& attackerShapes = attacker.GetShapes();
	const std::vector<HitShape>& victimShapes = victim.GetShapes();

	for (size_t groupIndex = 0; groupIndex < groups.size(); groupIndex++)
	{
		if (!groups[groupIndex].isActive || !attacker.CanHit(groupIndex, victim.owner, timeSec))
		{
			continue;
		}

		// one hit per group per victim
		const HitShape* bestHurtbox = nullptr;
		CapsuleContact3D bestContact;

		for (const HitShape& hitbox : attackerShapes)
		{
			if (hitbox.groupIndex != groupIndex || !hitbox.hasWorld)
			{
				continue;
			}

			for (const HitShape& hurtbox : victimShapes)
			{
				if (hurtbox.desc.role != HitVolumeRole::HURTBOX || !hurtbox.isEnabled)
				{
					continue;
				}

				const CapsuleContact3D contact = TestHitbox(hitbox, hurtbox.world);
				if (contact.isOverlap && (!bestHurtbox || contact.depth > bestContact.depth))
				{
					bestHurtbox = &hurtbox;
					bestContact = contact;
				}
			}
		}

		if (!bestHurtbox)
		{
			continue;
		}

		attacker.RecordHit(groupIndex, victim.owner, timeSec);
		outHits.push_back({
			attacker.owner, victim.owner, groups[groupIndex].name, bestHurtbox->desc.name,
			bestContact.contactPoint, groups[groupIndex].activationCount
		});
	}
}
