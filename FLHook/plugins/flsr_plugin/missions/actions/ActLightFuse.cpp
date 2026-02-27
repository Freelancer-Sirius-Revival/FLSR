#include "ActLightFuse.h"

namespace Missions
{
	static void Fuse(const ActLightFuse& action, uint objId)
	{
		if (pub::SpaceObj::ExistsAndAlive(objId) != 0)
			return;

		IObjRW* inspect;
		StarSystem* system;
		if (!GetShipInspect(objId, inspect, system))
			return;

		uint fuseId = action.fuse;
		inspect->light_fuse(0, &fuseId, 0, action.timeOffset, action.lifetimeOverride);
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
					Fuse(*this, objId);
			}
			else if (mission.objectIds.contains(activator.id))
				Fuse(*this, activator.id);
		}
		else
		{
			// Clients can only be addressed via Label.
			if (const auto& objectByName = mission.objectIdsByName.find(objNameOrLabel); objectByName != mission.objectIdsByName.end())
			{
				Fuse(*this, objectByName->second);
			}
			else if (const auto& objectsByLabel = mission.objectsByLabel.find(objNameOrLabel); objectsByLabel != mission.objectsByLabel.end())
			{
				for (const auto& object : objectsByLabel->second)
					Fuse(*this, object.id);
			}
		}
	}
}