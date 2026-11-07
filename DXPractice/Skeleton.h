#pragma once
#include <DirectXMath.h>
#include <string>
#include <unordered_map>
#include <vector>

// max number of bones allowed - MUST MATCH SHADER
constexpr unsigned maxBones = 128;

struct LocalTRS
{
	DirectX::XMFLOAT3 translation{};
	DirectX::XMFLOAT4 rotation{ 0.f, 0.f, 0.f, 1.f };
	DirectX::XMFLOAT3 scale{ 1.f, 1.f, 1.f };
};

struct SkeletonNode
{
	std::string name;
	int parent = -1; // parent node, -1 = root
	DirectX::XMFLOAT4X4 bindLocal{}; // local matrix

	// node TRS
	LocalTRS bindTRS;
};

struct SkeletonBone
{
	int node = -1;
	DirectX::XMFLOAT4X4 offset{}; // mesh space -> local space
};

struct Skeleton
{
	std::vector<SkeletonNode> nodes;
	std::vector<SkeletonBone> bones;
	std::unordered_map<std::string, int> nodeIndexByName;
	DirectX::XMFLOAT4X4 meshSpaceInverse{}; // world to mesh space conversion 
};
