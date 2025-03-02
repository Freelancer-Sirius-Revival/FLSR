#include "../../Main.h"
#include "ActSpawnSolar.h"

namespace Missions
{
	ActSpawnSolar::ActSpawnSolar(Trigger* parentTrigger, const ActSpawnSolarArchetype* archetype) :
		Action(parentTrigger, TriggerAction::Act_SpawnSolar),
		solarName(archetype->solarName)
	{}

	void ActSpawnSolar::Execute()
	{
		ConPrint(stows(trigger->mission->name) + L"->" + stows(trigger->name) + L": Act_SpawnSolar " + stows(solarName));
		for (auto it = trigger->mission->archetype.solars.begin(); it != trigger->mission->archetype.solars.end(); it++)
		{
			if (it->name == solarName)
			{
				const uint objId = SolarSpawn::SpawnSolarByName(trigger->mission->name + ":" + solarName);
				if (objId)
				{
					MissionObject object;
					object.id = objId;
					object.name = it->name;
					object.labels = std::unordered_set(it->labels);
					trigger->mission->objects.push_back(object);
				}
				ConPrint(L"\n");
				return;
			}
		}
		ConPrint(L" NOT FOUND\n");
	}
}