#include "../../Main.h"
#include "ActSpawnSolar.h"

namespace Missions
{
	ActSpawnSolar::ActSpawnSolar(const ActionParent& parent, const ActSpawnSolarArchetypePtr actionArchetype) :
		Action(parent, TriggerAction::Act_SpawnSolar),
		archetype(actionArchetype)
	{}

	void ActSpawnSolar::Execute()
	{
		ConPrint(stows(missions[parent.missionId].archetype->name) + L"->" + stows(triggers[parent.triggerId].archetype->name) + L": Act_SpawnSolar " + stows(archetype->solarName));
		if (missions[parent.missionId].objectIdsByName.contains(CreateID(archetype->solarName.c_str())))
		{
			ConPrint(L" ALREADY EXISTS\n");
			return;
		}
		for (auto it = missions[parent.missionId].archetype->solars.begin(); it != missions[parent.missionId].archetype->solars.end(); it++)
		{
			const auto& solar = **it;
			if (solar.name == archetype->solarName)
			{
				const uint objId = SolarSpawn::SpawnSolarByName(missions[parent.missionId].archetype->name + ":" + solar.name);
				if (objId)
				{
					missions[parent.missionId].objectIds.insert(objId);
					missions[parent.missionId].objectIdsByName[CreateID(solar.name.c_str())] = objId;
					for (const auto& label : solar.labels)
					{
						MissionObject object;
						object.type = MissionObjectType::Object;
						object.id = objId;
						missions[parent.missionId].objectsByLabel[label].push_back(object);
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