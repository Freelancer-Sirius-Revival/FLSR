#include "Main.h"
#include "ActSpawnSolar.h"

namespace Missions
{
	ActSpawnSolar::ActSpawnSolar(Trigger* parentTrigger, const ActSpawnSolarArchetype* archetype) :
		Action(parentTrigger, TriggerAction::Act_SpawnSolar),
		solarName(archetype->solarName)
	{}

	void ActSpawnSolar::Execute()
	{
		const std::wstring outputPretext = stows(trigger->mission->name) + L"->" + stows(trigger->name) + L": Act_SpawnSolar " + stows(solarName);
		for (auto it = trigger->mission->archetype.solars.begin(); it != trigger->mission->archetype.solars.end(); it++)
		{
			if (it->name == solarName)
			{
				ConPrint(outputPretext + L"\n");
				const uint objId = SolarSpawn::SpawnSolarByName(solarName);
				if (objId)
				{
					MissionObject object;
					object.id = objId;
					object.name = it->name;
					object.labels = std::unordered_set(it->labels);
					trigger->mission->objects.push_back(object);
				}
				return;
			}
		}
		ConPrint(outputPretext + L" NOT FOUND\n");
	}
}