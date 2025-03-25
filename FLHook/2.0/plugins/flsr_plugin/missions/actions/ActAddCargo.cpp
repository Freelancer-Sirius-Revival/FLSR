#include <FLHook.h>
#include "ActAddCargo.h"
#include "../Mission.h"

namespace Missions
{
	ActAddCargo::ActAddCargo(const ActionParent& parent, const ActAddCargoArchetypePtr actionArchetype) :
		Action(parent, ActionType::Act_LightFuse),
		archetype(actionArchetype)
	{}

	static bool AddCargo(const uint clientId, const ActAddCargoArchetype& archetype)
	{
		float remainingHold;
		pub::Player::GetRemainingHoldSize(clientId, remainingHold);
		const auto& item = Archetype::GetEquipment(archetype.itemId);
		if (item && (remainingHold >= item->fVolume * archetype.count))
		{
			pub::Player::AddCargo(clientId, archetype.itemId, archetype.count, 1.0f, archetype.missionFlagged);
			return true;
		}
		return false;
	}

	void ActAddCargo::Execute()
	{
		auto& mission = missions.at(parent.missionId);
		const auto& trigger = mission.triggers.at(parent.triggerId);
		ConPrint(stows(mission.archetype->name) + L"->" + stows(trigger.archetype->name) + L": Act_AddCargo " + std::to_wstring(archetype->itemId) + L" on " + std::to_wstring(archetype->objNameOrLabel));
		if (archetype->objNameOrLabel == Activator)
		{
			const auto& activator = trigger.activator;
			if (activator.type == MissionObjectType::Client)
			{
				AddCargo(activator.id, *archetype);
				ConPrint(L" client[" + std::to_wstring(activator.id) + L"]");
			}
		}
		else if (const auto& objectsByLabel = mission.objectsByLabel.find(archetype->objNameOrLabel); objectsByLabel != mission.objectsByLabel.end())
		{
			for (const auto& object : objectsByLabel->second)
			{
				if (object.type == MissionObjectType::Client)
				{
					AddCargo(object.id, *archetype);
					ConPrint(L" client[" + std::to_wstring(object.id) + L"]");
				}
			}
		}
		ConPrint(L"\n");
	}
}