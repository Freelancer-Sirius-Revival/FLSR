#include "ActAddCargo.h"

namespace Missions
{
	static bool AddCargo(const uint clientId, const ActAddCargo& action)
	{
		float remainingHold;
		pub::Player::GetRemainingHoldSize(clientId, remainingHold);
		const auto& item = Archetype::GetEquipment(action.itemId);
		if (item && (remainingHold >= item->fVolume * action.count))
		{
			pub::Player::AddCargo(clientId, action.itemId, action.count, 1.0f, action.missionFlagged);
			return true;
		}
		return false;
	}

	void ActAddCargo::Execute(Mission& mission, const MissionObject& activator) const
	{
		if (objNameOrLabel == Activator)
		{
			if (activator.type == MissionObjectType::Client)
			{
				AddCargo(activator.id, *this);
			}
		}
		else if (const auto& objectsByLabel = mission.objectsByLabel.find(objNameOrLabel); objectsByLabel != mission.objectsByLabel.end())
		{
			for (const auto& object : objectsByLabel->second)
			{
				if (object.type == MissionObjectType::Client)
				{
					AddCargo(object.id, *this);
				}
			}
		}
	}
}