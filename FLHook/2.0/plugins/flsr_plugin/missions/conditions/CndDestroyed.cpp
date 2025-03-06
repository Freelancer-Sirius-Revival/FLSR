#include "CndDestroyed.h"

namespace Missions
{
	std::unordered_set<CndDestroyed*> destroyedConditions;

	CndDestroyed::CndDestroyed(const ConditionParent& parent, const CndDestroyedArchetypePtr conditionArchetype) :
		Condition(parent, TriggerCondition::Cnd_Destroyed),
		archetype(conditionArchetype),
		count(0)
	{}

	void CndDestroyed::Register()
	{
		destroyedConditions.insert(this);
	}

	void CndDestroyed::Unregister()
	{
		destroyedConditions.erase(this);
	}

	bool CndDestroyed::Matches(const IObjRW* killedObject, const bool killed, const uint killerId)
	{
		// If count -1 and the awaited objects do not exist, see this condition as fulfilled. "Stranger" also is fulfilled then because otherwise all players must leave server for fulfill.
		if (archetype->count < 0 && (archetype->objNameOrLabel == Stranger || (!missions[parent.missionId].objectIdsByName.contains(archetype->objNameOrLabel) && !missions[parent.missionId].objectsByLabel.contains(archetype->objNameOrLabel))))
		{
			ConPrint(stows(missions[parent.missionId].archetype->name) + L"->" + stows(triggers[parent.triggerId].archetype->name) + L": Cnd_Destroyed " + std::to_wstring(archetype->objNameOrLabel) + L" none existing\n");
			return true;
		}
		
		// Check destruction conditions.
		if ((archetype->condition == DestroyedCondition::SILENT && killed) || (archetype->condition == DestroyedCondition::EXPLODE && !killed))
			return false;

		// Check if killed object is part of the mission.
		if (killedObject->is_player())
		{
			const bool containsPlayer = missions[parent.missionId].clientIds.contains(killedObject->cobj->ownerPlayer);
			if ((archetype->objNameOrLabel == Stranger && containsPlayer) || !containsPlayer)
				return false;
		}
		else
		{
			if (!missions[parent.missionId].objectIds.contains(killedObject->cobj->id))
				return false;
		}

		// Make sure the expected killer did the kill.
		if (archetype->killerNameOrLabel)
		{
			bool killerFound = false;
			if (const auto& objectByName = missions[parent.missionId].objectIdsByName.find(archetype->killerNameOrLabel); objectByName != missions[parent.missionId].objectIdsByName.end())
			{
				killerFound = objectByName->second == killerId;
			}
			else if (const auto& objectsByLabel = missions[parent.missionId].objectsByLabel.find(archetype->killerNameOrLabel); objectsByLabel != missions[parent.missionId].objectsByLabel.end())
			{
				for (const auto& object : objectsByLabel->second)
				{
					if (object.id == killerId)
					{
						killerFound = true;
						break;
					}
				}
			}
			if (!killerFound)
				return false;
		}

		byte foundObjectType = 0;
		if (killedObject->is_player() && archetype->objNameOrLabel == Stranger && !missions[parent.missionId].clientIds.contains(killedObject->cobj->ownerPlayer))
		{
			foundObjectType = 1;
		}
		// Clients can only be addressed via Label.
		else if (const auto& objectByName = missions[parent.missionId].objectIdsByName.find(archetype->objNameOrLabel); objectByName != missions[parent.missionId].objectIdsByName.end())
		{
			if (objectByName->second == killedObject->cobj->id)
			{
				foundObjectType = 2;
			}
		}
		else if (const auto& objectsByLabel = missions[parent.missionId].objectsByLabel.find(archetype->objNameOrLabel); objectsByLabel != missions[parent.missionId].objectsByLabel.end())
		{
			for (const auto& object : objectsByLabel->second)
			{
				if ((object.type == MissionObjectType::Client && object.id == killedObject->cobj->ownerPlayer) || (object.type == MissionObjectType::Object && object.id == killedObject->cobj->id))
				{
					foundObjectType = 3;
					break;
				}
			}
		}

		if (foundObjectType == 0)
			return false;

		ConPrint(stows(missions[parent.missionId].archetype->name) + L"->" + stows(triggers[parent.triggerId].archetype->name) + L": Cnd_Destroyed " + std::to_wstring(archetype->objNameOrLabel));
		bool foundAll = false;
		if (archetype->count < 0)
		{
			foundAll = foundObjectType <= 2 || (foundObjectType == 3 && missions[parent.missionId].objectsByLabel[archetype->objNameOrLabel].size() <= 1);
			ConPrint(L" all\n");
		}
		else
		{
			foundAll = ++count >= archetype->count;
			ConPrint(L" " + std::to_wstring(count) + L" of " + std::to_wstring(archetype->count) + L"\n");
		}

		if (foundAll)
		{
			const uint clientId = HkGetClientIDByShip(killerId);
			activator.type = clientId ? MissionObjectType::Client : MissionObjectType::Object;
			activator.id = clientId ? clientId : killerId;
			return true;
		}

		return false;
	}
}