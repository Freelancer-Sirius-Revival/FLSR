#pragma once
#include <iostream>
#include "TriggerArch.h"
#include "MsnSolarArch.h"
#include "NpcArch.h"
#include "Objectives/ObjectivesArch.h"

namespace Missions
{
	struct MissionArchetype
	{
		std::string name = "";
		bool active = false;
		std::vector<TriggerArchetypePtr> triggers;
		std::vector<MsnSolarArchetypePtr> solars;
		std::vector<NpcArchetypePtr> npcs;
		std::vector<MsnNpcArchetypePtr> msnNpcs;
		std::unordered_map<unsigned int, std::shared_ptr<ObjectivesArchetype>> objectives;
	};
	typedef std::shared_ptr<MissionArchetype> MissionArchetypePtr;

	extern std::vector<MissionArchetypePtr> missionArchetypes;
}
