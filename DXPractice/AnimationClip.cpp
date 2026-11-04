#include "AnimationClip.h"

#include <filesystem>
#include <memory>
#include <assimp/anim.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "DebugRenderer.h"
#include "Skeleton.h"

struct aiVectorKey;

namespace
{
	// fallback
	constexpr float defaultTicksPerSecond = 30.f;

	VectorKeyInfo ParseKeyInfo(const aiVectorKey& key)
	{
		return {static_cast<float>(key.mTime), {key.mValue.x, key.mValue.y, key.mValue.z}};
	}

	QuatKeyInfo ParseKeyInfo(const aiQuatKey& key)
	{
		// assimp: w first, dx: x first
		return {static_cast<float>(key.mTime), {key.mValue.x, key.mValue.y, key.mValue.z, key.mValue.w}};
	}
}

std::shared_ptr<const AnimationClip> LoadAnimationClip(const std::string& path, const Skeleton& skeleton)
{
	Assimp::Importer importer;
	importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
	const aiScene* scene = importer.ReadFile(path, aiProcess_ConvertToLeftHanded);

	if (!scene || scene->mNumAnimations == 0)
	{
		OutputDebugStringA(("[AnimationClip] no animation in " + path + "\n").c_str());
		OutputDebugStringA(importer.GetErrorString());
		assert(false);
		return nullptr;
	}

	const aiAnimation* anim = scene->mAnimations[0];

	auto clip = std::make_shared<AnimationClip>();
	clip->name = std::filesystem::path(path).stem().string();
	clip->duration = static_cast<float>(anim->mDuration);
	// read tps if 0 then use fallback
	clip->ticksPerSecond = anim->mTicksPerSecond > 0.0f
		                       ? static_cast<float>(anim->mTicksPerSecond)
		                       : defaultTicksPerSecond;
	clip->builtFor = &skeleton;

	clip->channels.resize(anim->mNumChannels);
	clip->channelToNode.assign(anim->mNumChannels, -1);

	unsigned mapped = 0;
	for (unsigned channel = 0; channel < anim->mNumChannels; channel++)
	{
		const aiNodeAnim* src = anim->mChannels[channel];
		AnimationChannel& dest = clip->channels[channel];
		dest.nodeName = src->mNodeName.C_Str();

		// parse key info
		dest.positions.reserve(src->mNumPositionKeys);
		for (unsigned key = 0; key < src->mNumPositionKeys; key++)
		{
			dest.positions.push_back(ParseKeyInfo(src->mPositionKeys[key]));
		}
		dest.rotations.reserve(src->mNumRotationKeys);
		for (unsigned key = 0; key < src->mNumRotationKeys; key++)
		{
			dest.rotations.push_back(ParseKeyInfo(src->mRotationKeys[key]));
		}
		dest.scales.reserve(src->mNumScalingKeys);
		for (unsigned key = 0; key < src->mNumScalingKeys; key++)
		{
			dest.scales.push_back(ParseKeyInfo(src->mScalingKeys[key]));
		}

		// map node index by name
		auto node = skeleton.nodeIndexByName.find(dest.nodeName);
		if (node != skeleton.nodeIndexByName.end())
		{
			clip->channelToNode[channel] = node->second;
			mapped++;
		}
		else
		{
			char msg[256];
			sprintf_s(msg, "[AnimationClip] %s: channel '%s' has no node in the skeleton (ignored)\n",
				clip->name.c_str(), dest.nodeName.c_str());
			OutputDebugStringA(msg);
		}
	}

	// debug msg
	char msg[256];
	sprintf_s(msg, "[AnimationClip] %s: %u/%u channels mapped, %.1f ticks @ %.1f tps (%.2fs)%s\n",
		clip->name.c_str(), mapped, anim->mNumChannels, clip->duration, clip->ticksPerSecond,
		clip->DurationInSeconds(), anim->mTicksPerSecond > 0.0 ? "" : " [tps fallback]");
	OutputDebugStringA(msg);

	return clip;
}
