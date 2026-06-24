#pragma once
#include "IComponent.h"

enum class ColliderType
{
	Box
};

struct AABB
{
	DirectX::XMFLOAT2 min;
	DirectX::XMFLOAT2 max;

	bool IsOverlap(const AABB& other) const
	{
		return min.x <= other.max.x && max.x >= other.min.x &&
			min.y >= other.max.y && max.y <= other.min.y;
	}
};

struct CollisionManifold
{
	DirectX::XMFLOAT2 normal;
	float penetration;
};

class Collider2D : public IComponent
{
public:
	virtual ColliderType GetType() const = 0;
	virtual AABB GetWorldAABB() const = 0;

	bool IsTrigger() const
	{
		return isTrigger;
	}

	void SetTrigger(const bool inIsTrigger)
	{
		isTrigger = inIsTrigger;
	}


	bool IsColliderActive() const
	{
		return isActive;
	}

	void SetColliderActive (bool inIsActive)
	{
		isActive = inIsActive;
	}

protected:
	bool isTrigger = false;
	bool isActive = true;
};

