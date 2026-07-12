#include "ActAddCargo.h"

namespace Missions
{
	static void AddCargo(const uint clientId, const ActAddCargo& action)
	{
		float remainingHold;
		pub::Player::GetRemainingHoldSize(clientId, remainingHold);
		const auto& item = Archetype::GetEquipment(action.itemId);
		if (item && (remainingHold >= item->fVolume * action.count))
		{
			const GoodInfo* good = GoodList::find_by_id(action.itemId);
			if (!good)
				return;
			if (good->multiCount)
				pub::Player::AddCargo(clientId, action.itemId, action.count, 1.0f, action.missionFlagged);
			else
				for (uint index = 0; index < action.count; index++)
					pub::Player::AddCargo(clientId, action.itemId, 1, 1.0f, action.missionFlagged);
		}
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