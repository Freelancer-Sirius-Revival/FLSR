#pragma once
#include <iostream>
#include "TriggerArch.h"
#include "MsnSolarArch.h"
#include "NpcArch.h"
#include "Objectives/ObjectivesArch.h"

namespace Missions
{
	struct MissionOffer
	{
		pub::GF::MissionType type = pub::GF::MissionType::Unknown;
		uint system = 0;
		uint group = 0;
		uint text = 0;
		uint reward = 0;
		std::vector<uint> bases;
	};

	struct MissionArchetype
	{
		std::string name = "";
		bool active = false;
		MissionOffer offer;
		std::vector<TriggerArchetypePtr> triggers;
		std::vector<MsnSolarArchetypePtr> solars;
		std::vector<NpcArchetypePtr> npcs;
		std::vector<MsnNpcArchetypePtr> msnNpcs;
		std::unordered_map<unsigned int, ObjectivesArchetypePtr> objectives;
	};
	typedef std::shared_ptr<MissionArchetype> MissionArchetypePtr;

	extern std::vector<MissionArchetypePtr> missionArchetypes;
}
