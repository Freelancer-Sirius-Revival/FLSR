#include "../../Main.h"
#include "ActSpawnSolar.h"
#include "../Mission.h"

namespace Missions
{
	ActSpawnSolar::ActSpawnSolar(const ActionParent& parent, const ActSpawnSolarArchetypePtr actionArchetype) :
		Action(parent, ActionType::Act_SpawnSolar),
		archetype(actionArchetype)
	{}

	void ActSpawnSolar::Execute()
	{
		auto& mission = missions.at(parent.missionId);
		const auto& trigger = mission.triggers.at(parent.triggerId);
		ConPrint(stows(mission.archetype->name) + L"->" + stows(trigger.archetype->name) + L": Act_SpawnSolar " + stows(archetype->solarName));
		if (mission.objectIdsByName.contains(CreateID(archetype->solarName.c_str())))
		{
			ConPrint(L" ALREADY EXISTS\n");
			return;
		}
		for (auto it = mission.archetype->solars.begin(); it != mission.archetype->solars.end(); it++)
		{
			const auto& solar = **it;
			if (solar.name == archetype->solarName)
			{
				const uint objId = SolarSpawn::SpawnSolarByName(mission.archetype->name + ":" + solar.name);
				if (objId)
				{
					mission.AddObject(objId, CreateID(solar.name.c_str()), solar.labels);
					ConPrint(L" obj[" + std::to_wstring(objId) + L"]");
				}
				ConPrint(L"\n");
				return;
			}
		}
		ConPrint(L" NOT FOUND\n");
	}
}