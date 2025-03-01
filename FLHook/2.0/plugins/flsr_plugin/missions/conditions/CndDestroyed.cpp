#include "CndDestroyed.h"

namespace Missions
{
	std::unordered_set<CndDestroyed*> destroyedConditions;

	CndDestroyed::CndDestroyed(Trigger* parentTrigger, const CndDestroyedArchetype* archetype) :
		Condition(parentTrigger, TriggerCondition::Cnd_Destroyed),
		count(0),
		objNameOrLabel(archetype->objNameOrLabel),
		targetCount(archetype->count),
		condition(archetype->condition),
		killerNameOrLabel(archetype->killerNameOrLabel)
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
		switch (condition)
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

		if (!killerNameOrLabel.empty())
		{
			bool killerFound = false;
			for (const auto& object : trigger->mission->objects)
			{
				if (object.id == killerId && (object.name == killerNameOrLabel || object.labels.contains(killerNameOrLabel)))
				{
					killerFound = true;
					break;
				}
			}
			if (!killerFound)
				return false;
		}

		std::wstring outputPretext = stows(trigger->mission->name) + L"->" + stows(trigger->name) + L": Cnd_Destroyed " + stows(objNameOrLabel);

		if (targetCount < 0)
		{
			int foundObjectCount = 0;
			for (const auto& object : trigger->mission->objects)
			{
				const bool nameOrLabelMatch = object.name == objNameOrLabel || object.labels.contains(objNameOrLabel);
				if (killedObject->cobj->id == object.id && nameOrLabelMatch)
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
				return true;
			}
		}
		else
		{
			for (const auto& object : trigger->mission->objects)
			{
				if (killedObject->cobj->id == object.id && (object.name == objNameOrLabel || object.labels.contains(objNameOrLabel)))
				{
					count++;
					ConPrint(outputPretext + L" " + std::to_wstring(count) + L" of " + std::to_wstring(targetCount) + L"\n");
					break;
				}
			}

			if (count >= targetCount)
			{
				ConPrint(outputPretext + L"\n");
				return true;
			}
		}

		return false;
	}
}