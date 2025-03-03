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
		std::vector<TriggerArchetypePtr> triggers;
		std::vector<MsnSolarArchetypePtr> solars;
	};
	typedef std::shared_ptr<MissionArchetype> MissionArchetypePtr;

	extern std::vector<MissionArchetypePtr> missionArchetypes;
}
