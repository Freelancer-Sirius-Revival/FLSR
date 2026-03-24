#include "ActUnlightFuse.h"

namespace Missions
{
	static void Fuse(uint fuseId, const uint objId)
	{
		if (pub::SpaceObj::ExistsAndAlive(objId) != 0)
			return;

		IObjRW* inspect;
		StarSystem* system;
		if (!GetShipInspect(objId, inspect, system))
			return;

		inspect->unlight_fuse_unk(&fuseId, 0, 0.0f);
	}

	void ActUnlightFuse::Execute(Mission& mission, const MissionObject& activator) const
	{
		if (objNameOrLabel == Activator)
		{
			if (activator.type == MissionObjectType::Client)
			{
				uint objId;
				pub::Player::GetShip(activator.id, objId);
				if (objId)
					Fuse(fuse, objId);
			}
			else if (mission.objectIds.contains(activator.id))
				Fuse(fuse, activator.id);
		}
		else
		{
			// Clients can only be addressed via Label.
			if (const auto& objectByName = mission.objectIdsByName.find(objNameOrLabel); objectByName != mission.objectIdsByName.end())
			{
				Fuse(fuse, objectByName->second);
			}
			else if (const auto& objectsByLabel = mission.objectsByLabel.find(objNameOrLabel); objectsByLabel != mission.objectsByLabel.end())
			{
				for (const auto& object : objectsByLabel->second)
					Fuse(fuse, object.id);
			}
		}
	}
}