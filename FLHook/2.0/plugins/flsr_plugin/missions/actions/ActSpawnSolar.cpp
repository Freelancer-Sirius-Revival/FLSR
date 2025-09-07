#include "../../Main.h"
#include "ActSpawnSolar.h"
#include "../../NpcCloaking.h"

namespace Missions
{
	void ActSpawnSolar::Execute(Mission& mission, const MissionObject& activator) const
	{
		if (mission.objectIdsByName.contains(CreateID(solarName.c_str())))
			return;

		for (auto it = mission.solars.begin(); it != mission.solars.end(); it++)
		{
			const auto& solar = *it;
			if (solar.name == solarName)
			{
				const uint objId = SolarSpawn::SpawnSolarByName(mission.name + ":" + solar.name);
				if (objId)
				{
					mission.AddObject(objId, CreateID(solar.name.c_str()), solar.labels);
					NpcCloaking::RegisterObject(objId);
				}
				return;
			}
		}
	}
}