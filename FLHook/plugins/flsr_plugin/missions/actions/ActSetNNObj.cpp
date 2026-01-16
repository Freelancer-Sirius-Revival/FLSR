#include "ActSetNNObj.h"
#include "../ClientObjectives.h"

namespace Missions
{
	void ActSetNNObj::Execute(Mission& mission, const MissionObject& activator) const
	{
		ClientObjectives::Objective objective;
		objective.missionId = mission.id;
		objective.objId = targetObjName;
		objective.systemId = systemId;
		objective.position = position;
		objective.message = message;
		objective.bestPath = bestRoute;

		if (label == Activator)
		{
			if (activator.type == MissionObjectType::Client && activator.id)
				ClientObjectives::SetClientObjective(activator.id, objective);
		}
		else if (const auto& objectsByLabel = mission.objectsByLabel.find(label); objectsByLabel != mission.objectsByLabel.end())
		{
			for (const auto& object : objectsByLabel->second)
			{
				if (object.type == MissionObjectType::Client)
					ClientObjectives::SetClientObjective(object.id, objective);
			}
		}
	}
}