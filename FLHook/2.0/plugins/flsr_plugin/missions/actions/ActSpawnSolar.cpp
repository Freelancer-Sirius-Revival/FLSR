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
		for (auto it = trigger->mission->archetype->solars.begin(); it != trigger->mission->archetype->solars.end(); it++)
		{
			if ((**it).name == archetype->solarName)
			{
				const uint objId = SolarSpawn::SpawnSolarByName(trigger->mission->archetype->name + ":" + archetype->solarName);
				if (objId)
				{
					MissionObject object;
					object.objId = objId;
					object.name = CreateID((**it).name.c_str());
					object.labels = std::unordered_set((**it).labels);
					trigger->mission->objects.push_back(object);
				}
				ConPrint(L"\n");
				return;
			}
		}
		ConPrint(L" NOT FOUND\n");
	}
}