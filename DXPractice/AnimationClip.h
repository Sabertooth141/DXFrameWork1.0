#pragma once
#include <DirectXMath.h>
#include <memory>
#include <string>
#include <vector>

struct Skeleton;

// key info in vector
struct VectorKeyInfo
{
	float time = 0.f;
	DirectX::XMFLOAT3 value{};
};

// key info in quaternion
struct QuatKeyInfo
{
	float time = 0.f;
	DirectX::XMFLOAT4 value{ 0.f, 0.f, 0.f, 1.f };
};

// keys for one node, rig independent
struct AnimationChannel
{
	std::string nodeName;
	std::vector<VectorKeyInfo> positions;
	std::vector<QuatKeyInfo> rotations;
	std::vector<VectorKeyInfo> scales;
};

struct AnimationClip
{
	std::string name;
	float duration = 0.f; // in ticks
	float ticksPerSecond = 0.f;
	std::vector<AnimationChannel> channels;

	// channel index to node index -1 = no such node
	std::vector<int> channelToNode;
	const Skeleton* builtFor = nullptr;

	float DurationInSeconds() const
	{
		return duration / ticksPerSecond;
	}
};

std::shared_ptr<const AnimationClip> LoadAnimationClip(const std::string& path, const Skeleton& skeleton);

