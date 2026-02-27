#include "ActAdjRep.h"

namespace Missions
{
	void ActAdjRep::Execute(Mission& mission, const MissionObject& activator) const
	{
		if (objNameOrLabel == Activator)
		{
			if (activator.type == MissionObjectType::Client)
			{
				if (change == 0.0f)
					Empathies::ChangeReputationsByReason(activator.id, groupId, reason);
				else
					Empathies::ChangeReputationsByValue(activator.id, groupId, change);
			}
		}
		else if (const auto& objectsByLabel = mission.objectsByLabel.find(objNameOrLabel); objectsByLabel != mission.objectsByLabel.end())
		{
			std::vector<uint> clientIds;
			for (const auto& object : objectsByLabel->second)
			{
				if (object.type == MissionObjectType::Client)
					clientIds.push_back(object.id);
			}
			if (change == 0.0f)
			{
				for (const auto& id : clientIds)
					Empathies::ChangeReputationsByReason(id, groupId, reason);
			}
			else
			{
				for (const auto& id : clientIds)
					Empathies::ChangeReputationsByValue(id, groupId, change);
			}
		}
	}
}