#include "CndSpaceExit.h"

namespace Missions
{
	std::unordered_set<CndSpaceExit*> spaceExitConditions;

	CndSpaceExit::CndSpaceExit(Trigger* parentTrigger, const CndSpaceExitArchetypePtr conditionArchetype) :
		Condition(parentTrigger, TriggerCondition::Cnd_SpaceExit),
		archetype(conditionArchetype)
	{}

	void CndSpaceExit::Register()
	{
		spaceExitConditions.insert(this);
	}

	void CndSpaceExit::Unregister()
	{
		spaceExitConditions.erase(this);
	}

	bool CndSpaceExit::Matches(const uint clientId, const uint systemId)
	{
		const std::wstring outputPretext = stows(trigger->mission->archetype->name) + L"->" + stows(trigger->archetype->name) + L": Cnd_SpaceExit " + std::to_wstring(archetype->objNameOrLabel) + L" from " + std::to_wstring(archetype->systemId);

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