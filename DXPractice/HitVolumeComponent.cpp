#include "HitVolumeComponent.h"

#include "DebugRenderer.h"
#include "GameContext.h"
#include "GameObject.h"
#include "HitDetectionSystem.h"
#include "SkeletalAnimatorComponent.h"

namespace
{
	constexpr DirectX::XMFLOAT4 hurtboxColor = {0.f, 1.f, 0.f, 1.f};
	constexpr DirectX::XMFLOAT4 hitboxInactiveColor{ 0.45f, 0.f, 0.f, 1.f };

	constexpr float axisExtra = 0.2f; // poke axes out of the shape
}

void HitVolumeComponent::OnAttach(GameContext& context)
{
	IComponent::OnAttach(context);

	context.hitDetectionSys.Register(this);
}

int HitVolumeComponent::AddShape(const HitShapeDesc& desc)
{
	HitShape shape;
	shape.desc = desc;

	if (!desc.boneName.empty())
	{
		if (!animator)
		{
			animator = owner->GetComponent<SkeletalAnimatorComponent>();
		}
		assert(animator && "bone-attached hit shape needs a SkeletalAnimatorComponent");

		shape.nodeIndex = animator->FindNodeIndex(desc.boneName);
		assert(shape.nodeIndex >= 0 && "hit shape bone not found");
	}

	hitShapes.push_back(std::move(shape));
	return static_cast<int>(hitShapes.size() - 1);
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

		const XMFLOAT4 color = shape.desc.role == HitVolumeRole::HURTBOX ? hurtboxColor : hitboxInactiveColor;
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
		const XMFLOAT4 axisColors[3] = { { 1.f, 0.f, 0.f, 1.f },{ 0.f, 1.f, 0.f, 1.f },{ 0.f, 0.4f, 1.f, 1.f } };

		for (int i = 0; i < 3; i++)
		{
			XMFLOAT3 end;
			XMStoreFloat3(&end, origin + XMVector3Rotate(axes[i], rotation) * length);
			debugRenderer.DrawLine3D(shape.bonePos, end, axisColors[i]);
		}
	}
}
