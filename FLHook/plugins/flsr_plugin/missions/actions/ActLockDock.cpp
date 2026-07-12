#include "ActLockDock.h"

namespace Missions
{
	static void ChangeLockState(Mission& mission, const uint clientId, const uint solarId, const bool lock)
	{
		if (auto entry = mission.lockedDocksByClientId.find(clientId); entry != mission.lockedDocksByClientId.end())
		{
			if (lock)
				entry->second.insert(solarId);
			else
				entry->second.erase(solarId);
		}
		else if (lock)
			mission.lockedDocksByClientId[clientId].insert(solarId);
	}

	void ActLockDock::Execute(Mission& mission, const MissionObject& activator) const
	{
		if (label == Activator)
		{
			if (activator.type == MissionObjectType::Client && activator.id)
				ChangeLockState(mission, activator.id, solarId, lock);
		}
		else if (const auto& objectsByLabel = mission.objectsByLabel.find(label); objectsByLabel != mission.objectsByLabel.end())
		{
			for (const auto& object : objectsByLabel->second)
			{
				if (object.type == MissionObjectType::Client)
					ChangeLockState(mission, object.id, solarId, lock);
			}
		}
	}
}