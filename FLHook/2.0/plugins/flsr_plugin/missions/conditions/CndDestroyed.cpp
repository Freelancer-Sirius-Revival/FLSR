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
		switch (archetype->condition)
		{
			case DestroyedCondition::SILENT:
			{
				if (killed)
					return false;
				break;
			}

			case DestroyedCondition::EXPLODE:
			{
				if (!killed)
					return false;
				break;
			}

			default:
				break;
		}

		if (!archetype->killerNameOrLabel)
		{
			bool killerFound = false;
			for (const auto& object : trigger->mission->objects)
			{
				if (object.objId == killerId && (object.name == archetype->killerNameOrLabel || object.labels.contains(archetype->killerNameOrLabel)))
				{
					killerFound = true;
					break;
				}
			}
			if (!killerFound)
				return false;
		}

		std::wstring outputPretext = stows(trigger->mission->archetype->name) + L"->" + stows(trigger->archetype->name) + L": Cnd_Destroyed " + std::to_wstring(archetype->objNameOrLabel);

		if (archetype->count < 0)
		{
			int foundObjectCount = 0;
			for (const auto& object : trigger->mission->objects)
			{
				const bool nameOrLabelMatch = object.name == archetype->objNameOrLabel || object.labels.contains(archetype->objNameOrLabel);
				if (killedObject->cobj->id == object.objId && nameOrLabelMatch)
				{
					// Reduce the count of objects by this label because this object was just destroyed.
					foundObjectCount--;
				}
				
				// Count the remaining alive objects by this label.
				if (nameOrLabelMatch)
					foundObjectCount++;
			}

			if (foundObjectCount <= 0)
			{
				ConPrint(outputPretext + L"\n");
				activator.objId = killerId;
				activator.clientId = HkGetClientIDByShip(killerId);
				return true;
			}
		}
		else
		{
			for (const auto& object : trigger->mission->objects)
			{
				if (killedObject->cobj->id == object.objId && (object.name == archetype->objNameOrLabel || object.labels.contains(archetype->objNameOrLabel)))
				{
					count++;
					ConPrint(outputPretext + L" " + std::to_wstring(count) + L" of " + std::to_wstring(archetype->count) + L"\n");
					break;
				}
			}

			if (count >= archetype->count)
			{
				ConPrint(outputPretext + L"\n");
				activator.objId = killerId;
				activator.clientId = HkGetClientIDByShip(killerId);
				return true;
			}
		}

		return false;
	}
}