#include "CndBaseEnter.h"
#include "../Trigger.h"

namespace Missions
{
	std::unordered_set<CndBaseEnter*> baseEnterConditions;

	CndBaseEnter::CndBaseEnter(const ConditionParent& parent, const CndBaseEnterArchetypePtr conditionArchetype) :
		Condition(parent, TriggerCondition::Cnd_BaseEnter),
		archetype(conditionArchetype)
	{}

	void CndBaseEnter::Register()
	{
		baseEnterConditions.insert(this);
	}

	void CndBaseEnter::Unregister()
	{
		baseEnterConditions.erase(this);
	}

	bool CndBaseEnter::Matches(const uint clientId, const uint baseId)
	{
		const std::wstring outputPretext = stows(missions[parent.missionId].archetype->name) + L"->" + stows(triggers[parent.triggerId].archetype->name) + L": Cnd_BaseEnter " + std::to_wstring(archetype->objNameOrLabel) + L" on " + std::to_wstring(archetype->baseId);

		if (archetype->objNameOrLabel == Stranger)
		{
			if (!missions[parent.missionId].clientIds.contains(clientId) && (!archetype->baseId || archetype->baseId == baseId))
			{
				triggers[parent.triggerId].activator.type = MissionObjectType::Client;
				triggers[parent.triggerId].activator.id = clientId;
				ConPrint(outputPretext + L" client[" + std::to_wstring(clientId) + L"]\n");
				return true;
			}
		}
		else if (const auto& objectsByLabel = missions[parent.missionId].objectsByLabel.find(archetype->objNameOrLabel); objectsByLabel != missions[parent.missionId].objectsByLabel.end())
		{
			for (const auto& object : objectsByLabel->second)
			{
				if (object.type == MissionObjectType::Client && object.id == clientId && (!archetype->baseId || archetype->baseId == baseId))
				{
					triggers[parent.triggerId].activator = object;
					ConPrint(outputPretext + L" client[" + std::to_wstring(object.id) + L"]\n");
					return true;
				}
			}
		}
		return false;
	}
}