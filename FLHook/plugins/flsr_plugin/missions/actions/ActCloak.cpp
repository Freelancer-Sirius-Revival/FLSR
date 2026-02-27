#include "ActCloak.h"
#include "../../NpcCloaking.h"

namespace Missions
{
	void ActCloak::Execute(Mission& mission, const MissionObject& activator) const
	{
		if (objNameOrLabel == Activator)
		{
			if (activator.type == MissionObjectType::Object)
				NpcCloaking::SetTargetCloakState(activator.id, cloaked);
		}
		else
		{
			if (const auto& objectByName = mission.objectIdsByName.find(objNameOrLabel); objectByName != mission.objectIdsByName.end())
			{
				NpcCloaking::SetTargetCloakState(objectByName->second, cloaked);
			}
			else if (const auto& objectsByLabel = mission.objectsByLabel.find(objNameOrLabel); objectsByLabel != mission.objectsByLabel.end())
			{
				for (const auto& object : objectsByLabel->second)
				{
					if (object.type == MissionObjectType::Object)
						NpcCloaking::SetTargetCloakState(object.id, cloaked);
				}
			}
		}
	}
}