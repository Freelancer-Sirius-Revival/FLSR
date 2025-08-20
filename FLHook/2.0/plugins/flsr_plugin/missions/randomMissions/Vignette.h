#pragma once
#include <FLHook.h>

namespace RandomMissions
{
	enum class VignetteType
	{
		Open, Field, Exclusion
	};

	struct Vignette
	{
		uint systemId;
		Vector position;
		float radius;
		VignetteType type;
		std::unordered_set<uint> factionIds;
	};

	extern std::unordered_map<uint, std::vector<Vignette>> vignettesBySystemId;
}