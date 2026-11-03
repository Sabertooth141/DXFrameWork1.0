#include "SkeletalAnimatorComponent.h"

SkeletalAnimatorComponent::SkeletalAnimatorComponent(Renderer& renderer, std::shared_ptr<const Skeleton> inSkeleton) :
	skeleton(std::move(inSkeleton)),
	paletteCBuffer(renderer, palette, 1) // b1
{
	const size_t nodeCount = skeleton->nodes.size();
	localPose.resize(nodeCount);
	globalPose.resize(nodeCount);

	// init as bind pose
	for (size_t i = 0; i < nodeCount; i++)
	{
		localPose[i] = skeleton->nodes[i].bindLocal;
	}

	for (auto& boneMatrix : palette.bones)
	{
		DirectX::XMStoreFloat4x4(&boneMatrix, DirectX::XMMatrixIdentity());
	}

	ComputeGloabls();
	ComputePalette();

#ifdef _DEBUG
	// in bind pose every final matrix should be identity
	float maxError = 0.f;
	for (size_t b = 0; b < skeleton->bones.size(); b++)
	{
		for (int r = 0; r < 4; r++)
		{
			for (int c = 0; c < 4; c++)
			{
				const float expected = (r == c) ? 1.f : 0.f;
				maxError = (std::max)(maxError, std::fabs(palette.bones[b].m[r][c] - expected));
			}
		}
	}
	char msg[128];
	sprintf_s(msg, "[SkeletalAnimator] bind palette: max |final - I| = %g\n", maxError);
	OutputDebugStringA(msg);
#endif
}

void SkeletalAnimatorComponent::BindPalette(Renderer& renderer)
{
	paletteCBuffer.Update(renderer, palette);
	paletteCBuffer.Bind(renderer);
}

void SkeletalAnimatorComponent::ComputeGloabls()
{
	// parents first
	for (size_t i = 0; i < skeleton->nodes.size(); i++)
	{
		const DirectX::XMMATRIX local = DirectX::XMLoadFloat4x4(&localPose[i]); // load to matrix from 4x4
		const int parentIndex = skeleton->nodes[i].parent;

		const DirectX::XMMATRIX global = (parentIndex < 0)
			                                 ? local
			                                 : local * DirectX::XMLoadFloat4x4(&globalPose[parentIndex]);
		DirectX::XMStoreFloat4x4(&globalPose[i], global);
	}
}

void SkeletalAnimatorComponent::ComputePalette()
{
	const DirectX::XMMATRIX meshSpaceInverse = DirectX::XMLoadFloat4x4(&skeleton->meshSpaceInverse);

	for (size_t boneIndex = 0; boneIndex < skeleton->bones.size(); boneIndex++)
	{
		const SkeletonBone& bone = skeleton->bones[boneIndex];
		const DirectX::XMMATRIX finalMatrix =
			DirectX::XMLoadFloat4x4(&bone.offset) *
			DirectX::XMLoadFloat4x4(&globalPose[bone.node]) *
			meshSpaceInverse;

		DirectX::XMStoreFloat4x4(&palette.bones[boneIndex], DirectX::XMMatrixTranspose(finalMatrix));
	}
}
