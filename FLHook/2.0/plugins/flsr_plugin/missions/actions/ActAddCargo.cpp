#include <FLHook.h>
#include "ActAddCargo.h"
#include "../Conditions/Condition.h"

namespace Missions
{
	ActAddCargo::ActAddCargo(const ActionParent& parent, const ActAddCargoArchetypePtr actionArchetype) :
		Action(parent, TriggerAction::Act_LightFuse),
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
		ConPrint(stows(missions[parent.missionId].archetype->name) + L"->" + stows(triggers[parent.triggerId].archetype->name) + L": Act_AddCargo " + std::to_wstring(archetype->itemId) + L" on " + std::to_wstring(archetype->objNameOrLabel));
		if (archetype->objNameOrLabel == Activator)
		{
			const auto& activator = triggers[parent.triggerId].condition->activator;
			if (activator.type == MissionObjectType::Client)
			{
				AddCargo(activator.id, *archetype);
				ConPrint(L" client[" + std::to_wstring(activator.id) + L"]");
			}
		}
		else if (const auto& objectsByLabel = missions[parent.missionId].objectsByLabel.find(archetype->objNameOrLabel); objectsByLabel != missions[parent.missionId].objectsByLabel.end())
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