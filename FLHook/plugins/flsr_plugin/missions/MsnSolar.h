#pragma once
#include <FLHook.h>

namespace Missions
{
	struct MsnSolarCostume
	{
		uint headId = 0;
		uint bodyId = 0;
		std::vector<uint> accessoryIds;
	};

	struct MsnSolar
	{
		std::string name = "";
		uint archetypeId = 0;
		uint loadoutId = 0;
		uint idsName = 0;
		uint systemId = 0;
		Vector position;
		Matrix orientation;
		uint baseId = 0;
		std::string faction = "";
		uint pilotId = 0;
		int hitpoints = -1;
		uint voiceId = 0;
		MsnSolarCostume costume;
		std::unordered_set<uint> labels;
	};
}
