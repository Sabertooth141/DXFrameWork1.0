#include "PhysicsSystem.h"

#include "CollisionDispatch.h"
#include "GameObject.h"

void PhysicsSystem::Register(Rigidbody2DComponent* rigidbody, Collider2D* collider, GameObject* gameObject)
{
	entries.emplace_back(rigidbody, collider, gameObject);
}

void PhysicsSystem::Unregister(GameObject* gameObject)
{
	std::erase_if(entries, [gameObject](const Entry& entry)
	{
		return entry.gameObject == gameObject;
	});
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
	const float penetration = manifold.penetration;

	const float invMassSum = a->GetInvMass() + b->GetInvMass();

	// if both mass are 0
	if (invMassSum <= 0.f)
	{
		return;
	}

	const DirectX::XMFLOAT3 posA = a->GetTransformComp().GetPosition();
	const DirectX::XMFLOAT3 posB = b->GetTransformComp().GetPosition();

	const float correctionA = penetration * (a->GetInvMass() / invMassSum);
	const float correctionB = penetration * (b->GetInvMass() / invMassSum);

	a->GetTransformComp().SetPosition({
		posA.x - normal.x * correctionA,
		posA.y - normal.y * correctionA,
		posA.z
	});

	b->GetTransformComp().SetPosition({
		posB.x + normal.x * correctionB,
		posB.y + normal.y * correctionB,
		posB.z
	});

	// separate along normal
	const DirectX::XMFLOAT2 velA = a->GetVelocity();
	const DirectX::XMFLOAT2 velB = b->GetVelocity();
	const DirectX::XMFLOAT2 velDiff = {velB.x - velA.x, velB.y - velA.y};

	const float velAlongNormal = velDiff.x * normal.x + velDiff.y * normal.y;
	if (velAlongNormal > 0.f)
	{
		return; // alrdy separating
	}

	const float restitution = std::min(a->GetRestitution(), b->GetRestitution());
	const float impulseMag = -(1.0f + restitution) * velAlongNormal / invMassSum;

	const DirectX::XMFLOAT2 impulse = {impulseMag * normal.x, impulseMag * normal.y};

	a->SetVelocity({velA.x - impulse.x * a->GetInvMass(), velA.y - impulse.y * a->GetInvMass()});
	b->SetVelocity({velB.x + impulse.x * b->GetInvMass(), velB.y + impulse.y * b->GetInvMass()});
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
		return ColliderPair{ .a = a, .b = b };
	}

	return ColliderPair{ .a = b, .b = a };
}
