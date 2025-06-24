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

		if (objNameOrLabel == Activator)
		{
			if (activator.type == MissionObjectType::Client && activator.id)
			{
				ClientObjectives::SetClientObjective(activator.id, objective);
				ClientObjectives::SendClientObjectives(activator.id);
			}
		}
		else if (const auto& objectsByLabel = mission.objectsByLabel.find(objNameOrLabel); objectsByLabel != mission.objectsByLabel.end())
		{
			for (const auto& object : objectsByLabel->second)
			{
				if (object.type == MissionObjectType::Client)
				{
					ClientObjectives::SetClientObjective(object.id, objective);
					ClientObjectives::SendClientObjectives(object.id);
				}
			}
		}
	}
}