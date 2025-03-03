#include "CndDestroyed.h"

namespace Missions
{
	std::unordered_set<CndDestroyed*> destroyedConditions;

	CndDestroyed::CndDestroyed(Trigger* parentTrigger, const CndDestroyedArchetypePtr conditionArchetype) :
		Condition(parentTrigger, TriggerCondition::Cnd_Destroyed),
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
		if (archetype->count < 0 && (archetype->objNameOrLabel == Stranger || (!trigger->mission->objectIdsByName.contains(archetype->objNameOrLabel) && !trigger->mission->objectsByLabel.contains(archetype->objNameOrLabel))))
		{
			ConPrint(stows(trigger->mission->archetype->name) + L"->" + stows(trigger->archetype->name) + L": Cnd_Destroyed " + std::to_wstring(archetype->objNameOrLabel) + L" none existing\n");
			return true;
		}
		
		// Check destruction conditions.
		if ((archetype->condition == DestroyedCondition::SILENT && killed) || (archetype->condition == DestroyedCondition::EXPLODE && !killed))
			return false;

		// Check if killed object is part of the mission.
		if (killedObject->is_player())
		{
			const bool containsPlayer = trigger->mission->clientIds.contains(killedObject->cobj->ownerPlayer);
			if ((archetype->objNameOrLabel == Stranger && containsPlayer) || !containsPlayer)
				return false;
		}
		else
		{
			if (!trigger->mission->objectIds.contains(killedObject->cobj->id))
				return false;
		}

		// Make sure the expected killer did the kill.
		if (archetype->killerNameOrLabel)
		{
			bool killerFound = false;
			if (const auto& objectByName = trigger->mission->objectIdsByName.find(archetype->killerNameOrLabel); objectByName != trigger->mission->objectIdsByName.end())
			{
				killerFound = objectByName->second == killerId;
			}
			else if (const auto& objectsByLabel = trigger->mission->objectsByLabel.find(archetype->killerNameOrLabel); objectsByLabel != trigger->mission->objectsByLabel.end())
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
		if (killedObject->is_player() && archetype->objNameOrLabel == Stranger && !trigger->mission->clientIds.contains(killedObject->cobj->ownerPlayer))
		{
			foundObjectType = 1;
		}
		// Clients can only be addressed via Label.
		else if (const auto& objectByName = trigger->mission->objectIdsByName.find(archetype->objNameOrLabel); objectByName != trigger->mission->objectIdsByName.end())
		{
			if (objectByName->second == killedObject->cobj->id)
			{
				foundObjectType = 2;
			}
		}
		else if (const auto& objectsByLabel = trigger->mission->objectsByLabel.find(archetype->objNameOrLabel); objectsByLabel != trigger->mission->objectsByLabel.end())
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

		ConPrint(stows(trigger->mission->archetype->name) + L"->" + stows(trigger->archetype->name) + L": Cnd_Destroyed " + std::to_wstring(archetype->objNameOrLabel));
		bool foundAll = false;
		if (archetype->count < 0)
		{
			foundAll = foundObjectType <= 2 || (foundObjectType == 3 && trigger->mission->objectsByLabel[archetype->objNameOrLabel].size() <= 1);
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