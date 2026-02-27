#include "ActSetLifeTime.h"
#include "../ShipSpawning.h"
#include "../LifeTimes.h"

namespace Missions
{
	static void SetObjectLifeTime(uint objId, const float lifeTime, Mission& mission)
	{
		if (pub::SpaceObj::ExistsAndAlive(objId) != 0)
			return;

		IObjRW* inspect;
		StarSystem* system;
		if (!GetShipInspect(objId, inspect, system))
			return;

		if (inspect->cobj->objectClass == CObject::CSHIP_OBJECT)
			ShipSpawning::SetLifeTime(objId, lifeTime);
		else
			SetSolarLifeTime(objId, lifeTime);
	}

	void ActSetLifeTime::Execute(Mission& mission, const MissionObject& activator) const
	{
		if (objNameOrLabel == Activator)
		{
			if (activator.type == MissionObjectType::Object)
				SetObjectLifeTime(activator.id, lifeTime, mission);
		}
		else
		{
			// Clients can only be addressed via Label.
			if (const auto& objectByName = mission.objectIdsByName.find(objNameOrLabel); objectByName != mission.objectIdsByName.end())
			{
				SetObjectLifeTime(objectByName->second, lifeTime, mission);
			}
			else if (const auto& objectsByLabel = mission.objectsByLabel.find(objNameOrLabel); objectsByLabel != mission.objectsByLabel.end())
			{
				for (const auto& object : objectsByLabel->second)
				{
					if (object.type == MissionObjectType::Object)
						SetObjectLifeTime(object.id, lifeTime, mission);
				}
			}
		}
	}
}