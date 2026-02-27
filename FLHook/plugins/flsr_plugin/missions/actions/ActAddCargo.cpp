#include "ActAddCargo.h"

namespace Missions
{
	static void AddCargo(const uint clientId, const ActAddCargo& action)
	{
		float remainingHold;
		pub::Player::GetRemainingHoldSize(clientId, remainingHold);
		const auto& item = Archetype::GetEquipment(action.itemId);
		if (item && (remainingHold >= item->fVolume * action.count))
			// This being triggered directly after buying/selling may cause cheat-detection because the CASH change packet hasn't come through yet.
			HkAddCargo(ARG_CLIENTID(clientId), action.itemId, action.count, action.missionFlagged);
	}

	void ActAddCargo::Execute(Mission& mission, const MissionObject& activator) const
	{
		if (label == Activator)
		{
			if (activator.type == MissionObjectType::Client)
				AddCargo(activator.id, *this);
		}
		else if (const auto& objectsByLabel = mission.objectsByLabel.find(label); objectsByLabel != mission.objectsByLabel.end())
		{
			for (const auto& object : objectsByLabel->second)
			{
				if (object.type == MissionObjectType::Client)
					AddCargo(object.id, *this);
			}
		}
	}
}