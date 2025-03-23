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
		auto& trigger = mission.triggers.at(parent.triggerId);
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
					mission.objectIds.insert(objId);
					mission.objectIdsByName[CreateID(solar.name.c_str())] = objId;
					for (const auto& label : solar.labels)
					{
						MissionObject object;
						object.type = MissionObjectType::Object;
						object.id = objId;
						mission.objectsByLabel[label].push_back(object);
					}
					ConPrint(L" obj[" + std::to_wstring(objId) + L"]");
				}
				ConPrint(L"\n");
				return;
			}
		}
		ConPrint(L" NOT FOUND\n");
	}
}