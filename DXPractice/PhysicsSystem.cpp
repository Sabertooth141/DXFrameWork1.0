#include "PhysicsSystem.h"

#include "CollisionDispatch.h"
#include "GameObject.h"

void PhysicsSystem::Register(Rigidbody2DComponent* rigidbody, Collider2D* collider, GameObject* gameObject)
{
	rigidbody->SetInertia(collider->ComputeInertia(rigidbody->GetMass()));
	entries.emplace_back(rigidbody, collider, gameObject);
}

void PhysicsSystem::Unregister(GameObject* gameObject)
{
	std::erase_if(entries, [gameObject](const Entry& entry)
		{
			return entry.gameObject == gameObject;
		});

	const auto matchesPair = [gameObject](const ColliderPair& pair)
		{
			return pair.a == gameObject || pair.b == gameObject;
		};

	std::erase_if(previousOverlaps, matchesPair);
	std::erase_if(currentOverlaps, matchesPair);

	grid.clear();
}

void PhysicsSystem::Update(const float deltaTime)
{
	IntegrateForces(deltaTime);

	RebuildGrid();

	const std::vector<EntryPair> candidatePairs = GenerateCandidatePairs();

	currentOverlaps.clear();
	ProcessPairs(candidatePairs);

	for (const ColliderPair& pair : previousOverlaps)
	{
		if (!currentOverlaps.contains(pair))
		{
			pair.a->NotifyColliderLeave2D(*pair.b);
			pair.b->NotifyColliderLeave2D(*pair.a);
		}
	}

	previousOverlaps = currentOverlaps;

	for (Entry& entry : entries)
	{
		entry.rigidbody->ClearAccumulator();
	}
}

void PhysicsSystem::IntegrateForces(const float deltaTime)
{
	for (Entry& entry : entries)
	{
		entry.rigidbody->Integrate(deltaTime);
	}
}

void PhysicsSystem::RebuildGrid()
{
	grid.clear();

	for (Entry& entry : entries)
	{
		if (!entry.collider->IsColliderActive())
		{
			continue;
		}

		const AABB box = entry.collider->GetWorldAABB();

		const CellCoord minCell = GetCellCoord(box.min);
		const CellCoord maxCell = GetCellCoord(box.max);

		for (int cx = minCell.x; cx <= maxCell.x; cx++)
		{
			for (int cy = minCell.y; cy <= maxCell.y; cy++)
			{
				grid[CellCoord{cx, cy}].push_back(&entry);
			}
		}
	}
}

std::vector<PhysicsSystem::EntryPair> PhysicsSystem::GenerateCandidatePairs()
{
	std::unordered_set<EntryPair, EntryPairHash> dedup;

	// log every collider inside a cell thats not static
	for (const auto& [cell, entriesInCell] : grid)
	{
		for (size_t i = 0; i < entriesInCell.size(); i++)
		{
			for (size_t j = i + 1; j < entriesInCell.size(); j++)
			{
				Entry* a = entriesInCell[i];
				Entry* b = entriesInCell[j];

				// if both entries are static skip
				if (a->rigidbody->IsStatic() && b->rigidbody->IsStatic())
				{
					continue;
				}

				dedup.insert(MakeCanonicalEntryPair(a, b));
			}
		}
	}

	return std::vector<EntryPair>(dedup.begin(), dedup.end());
}

void PhysicsSystem::ProcessPairs(const std::vector<EntryPair>& candidates)
{
	for (const EntryPair& pair : candidates)
	{
		// if not close enough for SAT checks skip
		if (!pair.a->collider->GetWorldAABB().IsOverlap(pair.b->collider->GetWorldAABB()))
		{
			continue;
		}

		const std::optional<CollisionManifold> manifold = CollisionDispatch::GetInstance().Test(
			*pair.a->collider, *pair.b->collider);

		// if not touching 
		if (!manifold)
		{
			continue;
		}

		const ColliderPair colliderPair = MakeCanonicalColliderPair(pair.a->gameObject, pair.b->gameObject);
		currentOverlaps.insert(colliderPair);

		// event handling
		if (!previousOverlaps.contains(colliderPair))
		{
			pair.a->gameObject->NotifyColliderEnter2D(*pair.b->gameObject);
			pair.b->gameObject->NotifyColliderEnter2D(*pair.a->gameObject);
		}
		else
		{
			pair.a->gameObject->NotifyColliderStay2D(*pair.b->gameObject);
			pair.b->gameObject->NotifyColliderStay2D(*pair.a->gameObject);
		}

		// if either collider is trigger -> skips physics
		if (pair.a->collider->IsTrigger() || pair.b->collider->IsTrigger())
		{
			continue;
		}

		ResolveCollision(pair.a->rigidbody, pair.b->rigidbody, *manifold);
	}
}

void PhysicsSystem::ResolveCollision(Rigidbody2DComponent* a, Rigidbody2DComponent* b,
                                     const CollisionManifold& manifold)
{
	const DirectX::XMFLOAT2 normal = manifold.normal;
	const float invMassSum = a->GetInvMass() + b->GetInvMass();

	// if both masses are 0
	if (invMassSum <= 0.f)
	{
		return;
	}

	const DirectX::XMFLOAT3 posA = a->GetTransformComp().GetPosition();
	const DirectX::XMFLOAT3 posB = b->GetTransformComp().GetPosition();

	if (manifold.contactCount > 0)
	{
		const float restitution = std::min(a->GetRestitution(), b->GetRestitution());
		const float mu = std::sqrt(a->GetFriction() * b->GetFriction());

		// naive split so a two-point contact doesn't apply double the impulse
		const float share = 1.0f / static_cast<float>(manifold.contactCount);

		for (int i = 0; i < manifold.contactCount; i++)
		{
			const DirectX::XMFLOAT2 contact = manifold.contacts[i];
			const DirectX::XMFLOAT2 rA = {contact.x - posA.x, contact.y - posA.y};
			const DirectX::XMFLOAT2 rB = {contact.x - posB.x, contact.y - posB.y};

			// relative velocity at the contact, not at the centers
			const DirectX::XMFLOAT2 velA = a->GetVelAtPoint(rA);
			const DirectX::XMFLOAT2 velB = b->GetVelAtPoint(rB);
			const DirectX::XMFLOAT2 relVel = {velB.x - velA.x, velB.y - velA.y};

			const float velAlongNormal = relVel.x * normal.x + relVel.y * normal.y;
			if (velAlongNormal > 0.f)
			{
				continue; // alrdy separating at this contact
			}

			// effective mass along the normal, including the angular term (r x n)^2 * invI
			const float rACrossN = rA.x * normal.y - rA.y * normal.x;
			const float rBCrossN = rB.x * normal.y - rB.y * normal.x;
			const float normalMass = invMassSum
				+ rACrossN * rACrossN * a->GetInvInertia()
				+ rBCrossN * rBCrossN * b->GetInvInertia();

			const float impulseMag = (-(1.0f + restitution) * velAlongNormal / normalMass) * share;
			const DirectX::XMFLOAT2 impulse = {normal.x * impulseMag, normal.y * impulseMag};

			a->ApplyImpulse({-impulse.x, -impulse.y}, rA);
			b->ApplyImpulse(impulse, rB);

			// tangential impulse, this is what actually makes bodies tumble
			DirectX::XMFLOAT2 tangent = {
				relVel.x - normal.x * velAlongNormal,
				relVel.y - normal.y * velAlongNormal
			};

			const float tangentLen = std::sqrt(tangent.x * tangent.x + tangent.y * tangent.y);
			if (tangentLen < 1e-6f)
			{
				continue; // no sliding at this contact
			}

			tangent = {tangent.x / tangentLen, tangent.y / tangentLen};

			const float rACrossT = rA.x * tangent.y - rA.y * tangent.x;
			const float rBCrossT = rB.x * tangent.y - rB.y * tangent.x;
			const float tangentMass = invMassSum
				+ rACrossT * rACrossT * a->GetInvInertia()
				+ rBCrossT * rBCrossT * b->GetInvInertia();

			const float relAlongTangent = relVel.x * tangent.x + relVel.y * tangent.y;
			float frictionMag = (-relAlongTangent / tangentMass) * share;

			// coulomb clamp
			frictionMag = std::clamp(frictionMag, -impulseMag * mu, impulseMag * mu);

			const DirectX::XMFLOAT2 frictionImpulse = {tangent.x * frictionMag, tangent.y * frictionMag};

			a->ApplyImpulse({-frictionImpulse.x, -frictionImpulse.y}, rA);
			b->ApplyImpulse(frictionImpulse, rB);
		}
	}

	// positional correction once, with slop so resting bodies don't jitter
	constexpr float slop = 0.05f;
	constexpr float percent = 0.8f;
	const float correction = std::max(manifold.penetration - slop, 0.f) / invMassSum * percent;

	a->GetTransformComp().SetPosition({
		posA.x - normal.x * correction * a->GetInvMass(),
		posA.y - normal.y * correction * a->GetInvMass(),
		posA.z
	});

	b->GetTransformComp().SetPosition({
		posB.x + normal.x * correction * b->GetInvMass(),
		posB.y + normal.y * correction * b->GetInvMass(),
		posB.z
	});
}

PhysicsSystem::CellCoord PhysicsSystem::GetCellCoord(const DirectX::XMFLOAT2 worldPos) const
{
	return CellCoord{
		.x = static_cast<int>(std::floor(worldPos.x / cellSize)),
		.y = static_cast<int>(std::floor(worldPos.y / cellSize))
	};
}

PhysicsSystem::EntryPair PhysicsSystem::MakeCanonicalEntryPair(Entry* a, Entry* b)
{
	if (a < b)
	{
		return EntryPair{.a = a, .b = b};
	}

	return EntryPair{.a = b, .b = a};
}

PhysicsSystem::ColliderPair PhysicsSystem::MakeCanonicalColliderPair(GameObject* a, GameObject* b)
{
	if (a < b)
	{
		return ColliderPair{.a = a, .b = b};
	}

	return ColliderPair{.a = b, .b = a};
}
