#include "CndSpaceEnter.h"

namespace Missions
{
	std::unordered_set<CndSpaceEnter*> spaceEnterConditions;

	CndSpaceEnter::CndSpaceEnter(Trigger* parentTrigger, const CndSpaceEnterArchetypePtr conditionArchetype) :
		Condition(parentTrigger, TriggerCondition::Cnd_SpaceEnter),
		archetype(conditionArchetype)
	{}

	void CndSpaceEnter::Register()
	{
		spaceEnterConditions.insert(this);
	}

	void CndSpaceEnter::Unregister()
	{
		spaceEnterConditions.erase(this);
	}

	bool CndSpaceEnter::Matches(const uint clientId, const uint systemId)
	{
		const std::wstring outputPretext = stows(trigger->mission->archetype->name) + L"->" + stows(trigger->archetype->name) + L": Cnd_SpaceEnter " + std::to_wstring(archetype->objNameOrLabel) + L" into " + std::to_wstring(archetype->systemId);

		if (archetype->objNameOrLabel == Stranger)
		{
			if (!trigger->mission->clientIds.contains(clientId) && (!archetype->systemId || archetype->systemId == systemId))
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
				if (object.type == MissionObjectType::Client && object.id == clientId && (!archetype->systemId || archetype->systemId == systemId))
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