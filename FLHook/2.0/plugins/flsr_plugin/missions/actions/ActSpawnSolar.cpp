#include "../../Main.h"
#include "ActSpawnSolar.h"

namespace Missions
{
	ActSpawnSolar::ActSpawnSolar(Trigger* parentTrigger, const ActSpawnSolarArchetypePtr actionArchetype) :
		Action(parentTrigger, TriggerAction::Act_SpawnSolar),
		archetype(actionArchetype)
	{}

	void ActSpawnSolar::Execute()
	{
		ConPrint(stows(trigger->mission->archetype->name) + L"->" + stows(trigger->archetype->name) + L": Act_SpawnSolar " + stows(archetype->solarName));
		if (trigger->mission->objectIdsByName.contains(CreateID(archetype->solarName.c_str())))
		{
			ConPrint(L" ALREADY EXISTS\n");
			return;
		}
		for (auto it = trigger->mission->archetype->solars.begin(); it != trigger->mission->archetype->solars.end(); it++)
		{
			const auto& solar = **it;
			if (solar.name == archetype->solarName)
			{
				const uint objId = SolarSpawn::SpawnSolarByName(trigger->mission->archetype->name + ":" + solar.name);
				if (objId)
				{
					trigger->mission->objectIds.insert(objId);
					trigger->mission->objectIdsByName[CreateID(solar.name.c_str())] = objId;
					for (const auto& label : solar.labels)
					{
						MissionObject object;
						object.type = MissionObjectType::Object;
						object.id = objId;
						trigger->mission->objectsByLabel[label].push_back(object);
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