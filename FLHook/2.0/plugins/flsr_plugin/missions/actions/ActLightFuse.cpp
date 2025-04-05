#include "ActLightFuse.h"

namespace Missions
{
	static void Fuse(const std::string& fuseName, uint objId)
	{
		if (fuseName.empty() || pub::SpaceObj::ExistsAndAlive(objId) != 0)
			return;
		pub::SpaceObj::LightFuse(objId, fuseName.c_str(), 0.0f);
	}

	void ActLightFuse::Execute(Mission& mission, const MissionObject& activator) const
	{
		if (objNameOrLabel == Activator)
		{
			if (activator.type == MissionObjectType::Client)
			{
				uint objId;
				pub::Player::GetShip(activator.id, objId);
				if (objId)
					Fuse(fuseName, objId);
			}
			else if (mission.objectIds.contains(activator.id))
				Fuse(fuseName, activator.id);
		}
		else
		{
			// Clients can only be addressed via Label.
			if (const auto& objectByName = mission.objectIdsByName.find(objNameOrLabel); objectByName != mission.objectIdsByName.end())
			{
				Fuse(fuseName, objectByName->second);
			}
			else if (const auto& objectsByLabel = mission.objectsByLabel.find(objNameOrLabel); objectsByLabel != mission.objectsByLabel.end())
			{
				for (const auto& object : objectsByLabel->second)
					Fuse(fuseName, object.id);
			}
		}
	}
}