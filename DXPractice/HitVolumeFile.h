#pragma once
#include <string>

#include "HitVolumeComponent.h"

// JSON file <-> HitVolumeDesc (one file per character type, e.g. data/hitvolumes/dragon.json)
namespace HitVolumeFile
{
	struct Result
	{
		bool ok = false;
		bool fileFound = false;
		std::string message; // for the editor
	};

	Result Load(const std::string& path, HitVolumeDesc& out);
	Result Save(const std::string& path, const HitVolumeDesc& desc);
}