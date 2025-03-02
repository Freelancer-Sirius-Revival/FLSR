#pragma once
#include <iostream>
#include "TriggerArch.h"
#include "MsnSolarArch.h"

namespace Missions
{
	struct MissionArchetype
	{
		std::string name = "";
		bool active = false;
		int reward = 0;
		unsigned int titleId = 1;
		unsigned int offerId = 1;
		std::vector<TriggerArchetype> triggers;
		std::vector<MsnSolarArchetype> solars;
	};

	extern std::vector<MissionArchetype> missionArchetypes;
}
