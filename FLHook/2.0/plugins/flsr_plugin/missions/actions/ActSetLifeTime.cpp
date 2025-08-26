#include "ActSetLifeTime.h"
#include "../ShipSpawning.h"

namespace Missions
{
	void ActSetLifeTime::Execute(Mission& mission, const MissionObject& activator) const
	{
		if (objNameOrLabel == Activator)
		{
			if (activator.type == MissionObjectType::Object)
				SetLifeTime(activator.id, lifeTime);
		}
		else
		{
			// Clients can only be addressed via Label.
			if (const auto& objectByName = mission.objectIdsByName.find(objNameOrLabel); objectByName != mission.objectIdsByName.end())
			{
				SetLifeTime(objectByName->second, lifeTime);
			}
			else if (const auto& objectsByLabel = mission.objectsByLabel.find(objNameOrLabel); objectsByLabel != mission.objectsByLabel.end())
			{
				for (const auto& object : objectsByLabel->second)
				{
					if (object.type == MissionObjectType::Object)
						SetLifeTime(object.id, lifeTime);
				}
			}
		}
	}
}