#include "ActRemoveCargo.h"

namespace Missions
{
	static void RemoveCargo(const uint clientId, uint itemId, uint count)
	{
		if (!HkIsValidClientID(clientId))
			return;

		for (const auto& equip : Players[clientId].equipDescList.equip)
		{
			if (equip.iArchID == itemId)
			{
				pub::Player::RemoveCargo(clientId, equip.sID, std::min<uint>(count, equip.iCount));
				return;
			}
		}
	}

	void ActRemoveCargo::Execute(Mission& mission, const MissionObject& activator) const
	{
		if (label == Activator)
		{
			if (activator.type == MissionObjectType::Client)
				RemoveCargo(activator.id, itemId, count);
		}
		else if (const auto& objectsByLabel = mission.objectsByLabel.find(label); objectsByLabel != mission.objectsByLabel.end())
		{
			for (const auto& object : objectsByLabel->second)
			{
				if (object.type == MissionObjectType::Client)
					RemoveCargo(object.id, itemId, count);
			}
		}
	}
}