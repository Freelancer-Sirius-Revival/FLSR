#include "CndBaseEnter.h"

namespace Missions
{
	std::unordered_set<CndBaseEnter*> baseEnterConditions;

	CndBaseEnter::CndBaseEnter(Trigger* parentTrigger, const CndBaseEnterArchetypePtr conditionArchetype) :
		Condition(parentTrigger, TriggerCondition::Cnd_BaseEnter),
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
		const std::wstring outputPretext = stows(trigger->mission->archetype->name) + L"->" + stows(trigger->archetype->name) + L": Cnd_BaseEnter " + std::to_wstring(archetype->objNameOrLabel) + L" on " + std::to_wstring(archetype->baseId);

		if (archetype->objNameOrLabel == Stranger)
		{
			if (!trigger->mission->clientIds.contains(clientId) && (!archetype->baseId || archetype->baseId == baseId))
			{
				activator.type = MissionObjectType::Client;
				activator.id = clientId;
				ConPrint(outputPretext + L" client[" + std::to_wstring(clientId) + L"]\n");
				return true;
			}
		}
		else if (const auto& objectsByLabel = trigger->mission->objectsByLabel.find(archetype->objNameOrLabel); objectsByLabel != trigger->mission->objectsByLabel.end())
		{
			for (const auto& object : objectsByLabel->second)
			{
				if (object.type == MissionObjectType::Client && object.id == clientId && (!archetype->baseId || archetype->baseId == baseId))
				{
					activator = object;
					ConPrint(outputPretext + L" client[" + std::to_wstring(object.id) + L"]\n");
					return true;
				}
			}
		}
		return false;
	}
}