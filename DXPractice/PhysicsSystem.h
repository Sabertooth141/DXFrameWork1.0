#pragma once
#include <functional>
#include <unordered_set>

#include "Rigidbody2DComponent.h"

struct CollisionManifold;
class GameObject;
class Collider2D;

class PhysicsSystem
{
public:
	void Register(Rigidbody2DComponent* rigidbody, Collider2D* collider, GameObject* gameObject);

	void Unregister(GameObject* gameObject);

	void Update(float deltaTime);

private:
	// for spatial partitioning
	struct Entry
	{
		Rigidbody2DComponent* rigidbody;
		Collider2D* collider;
		GameObject* gameObject;
	};

	// grid 
	struct CellCoord
	{
		int x, y;

		bool operator==(const CellCoord& other) const
		{
			return x == other.x && y == other.y;
		}
	};

	struct CellCoordHash
	{
		size_t operator()(const CellCoord& cell) const
		{
			return std::hash<int>()(cell.x) ^ (std::hash<int>()(cell.y) << 1);
		}
	};

	// candidate pair for comparison
	struct EntryPair
	{
		Entry* a;
		Entry* b;

		bool operator==(const EntryPair& other) const
		{
			return a == other.a && b == other.b;
		}
	};

	struct EntryPairHash
	{
		size_t operator()(const EntryPair& pair) const
		{
			return std::hash<Entry*>()(pair.a) ^ (std::hash<Entry*>()(pair.b) << 1);
		}
	};

	// persistent pairs 
	struct ColliderPair
	{
		GameObject* a;
		GameObject* b;

		bool operator==(const ColliderPair& other) const
		{
			return a == other.a && b == other.b;
		}
	};

	struct ColliderPairHash
	{
		size_t operator()(const ColliderPair& pair) const
		{
			return std::hash<GameObject*>()(pair.a) ^ (std::hash<GameObject*>()(pair.b) << 1);
		}
	};

	// logic 

	void IntegrateForces(float deltaTime);
	void RebuildGrid();
	std::vector<EntryPair> GenerateCandidatePairs();
	void ProcessPairs(const std::vector<EntryPair>& candidates);
	void ResolveCollision(Rigidbody2DComponent* a, Rigidbody2DComponent* b, const CollisionManifold& manifold);

	// helpers

	CellCoord GetCellCoord(const DirectX::XMFLOAT2 worldPos);

	static EntryPair MakeCanonicalEntryPair(Entry* a, Entry* b);
	static ColliderPair MakeCanonicalColliderPair(GameObject* a, GameObject* b);

private:
	std::vector<Entry> entries;

	std::unordered_map<CellCoord, std::vector<Entry*>, CellCoordHash> grid;
	float cellSize = 64.f;

	std::unordered_set<ColliderPair, ColliderPairHash> currentOverlaps;
	std::unordered_set<ColliderPair, ColliderPairHash> previousOverlaps;
};
