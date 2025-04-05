#include "../../Main.h"
#include "ActSpawnSolar.h"

namespace Missions
{
	void ActSpawnSolar::Execute(Mission& mission, const MissionObject& activator) const
	{
		if (mission.objectIdsByName.contains(CreateID(solarName.c_str())))
			return;

		for (auto it = mission.archetype->solars.begin(); it != mission.archetype->solars.end(); it++)
		{
			const auto& solar = *it;
			if (solar.name == solarName)
			{
				const uint objId = SolarSpawn::SpawnSolarByName(mission.archetype->name + ":" + solar.name);
				if (objId)
					mission.AddObject(objId, CreateID(solar.name.c_str()), solar.labels);
				return;
			}
		}
	}
}