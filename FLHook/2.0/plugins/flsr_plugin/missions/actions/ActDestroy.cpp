#include "ActDestroy.h"

namespace Missions
{
	void ActDestroy::Execute(Mission& mission, const MissionObject& activator) const
	{
		if (objNameOrLabel == Activator)
		{
			uint objId;
			if (activator.type == MissionObjectType::Client)
				pub::Player::GetShip(activator.id, objId);
			else
				objId = activator.id;

			if (objId && pub::SpaceObj::ExistsAndAlive(objId) == 0)
				pub::SpaceObj::Destroy(objId, destroyType);
		}
		else
		{
			// Clients can only be addressed via Label.
			if (const auto& objectByName = mission.objectIdsByName.find(objNameOrLabel); objectByName != mission.objectIdsByName.end())
			{
				const uint objId = objectByName->second;
				if (pub::SpaceObj::ExistsAndAlive(objId) == 0)
					pub::SpaceObj::Destroy(objId, destroyType);
			}
			else if (const auto& objectsByLabel = mission.objectsByLabel.find(objNameOrLabel); objectsByLabel != mission.objectsByLabel.end())
			{
				// Copy list since the destruction of objects will in turn modify it via other hooks
				const std::vector<MissionObject> objectsCopy(objectsByLabel->second);
				for (const auto& object : objectsCopy)
				{
					uint objId;
					if (object.type == MissionObjectType::Client)
						pub::Player::GetShip(object.id, objId);
					else
						objId = object.id;

					if (objId && pub::SpaceObj::ExistsAndAlive(objId) == 0)
					{
						pub::SpaceObj::Destroy(objId, destroyType);
					}
				}
			}
		}
	}
}