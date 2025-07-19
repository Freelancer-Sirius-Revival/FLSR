#include "ActSpawnShip.h"
#include "MissionShipSpawning.h"
#include "../Objectives/Objectives.h"

namespace Missions
{
	void ActSpawnShip::Execute(Mission& mission, const MissionObject& activator) const
	{
		const uint objId = SpawnShip(msnNpcId, mission, position.x != std::numeric_limits<float>::infinity() ? &position : nullptr, orientation.data[0][0] != std::numeric_limits<float>::infinity() ? &orientation : nullptr);
		if (objId)
		{
			if (const auto& objectivesEntry = mission.objectives.find(objectivesId); objectivesEntry != mission.objectives.end())
			{
				mission.objectivesByObjectId.try_emplace(objId, mission.id, objId, objectivesEntry->second.objectives);
				mission.objectivesByObjectId.at(objId).Progress();
			}
		}
	}
}