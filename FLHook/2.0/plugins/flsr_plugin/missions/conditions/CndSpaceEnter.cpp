#include "CndSpaceEnter.h"

namespace Missions
{
	std::unordered_set<CndSpaceEnter*> spaceEnterConditions;

	CndSpaceEnter::CndSpaceEnter(const ConditionParent& parent, const CndSpaceEnterArchetypePtr conditionArchetype) :
		Condition(parent, TriggerCondition::Cnd_SpaceEnter),
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
		const std::wstring outputPretext = stows(missions[parent.missionId].archetype->name) + L"->" + stows(triggers[parent.triggerId].archetype->name) + L": Cnd_SpaceEnter " + std::to_wstring(archetype->objNameOrLabel) + L" into " + std::to_wstring(archetype->systemId);

		if (archetype->objNameOrLabel == Stranger)
		{
			if (!missions[parent.missionId].clientIds.contains(clientId) && (!archetype->systemId || archetype->systemId == systemId))
			{
				activator.type = MissionObjectType::Client;
				activator.id = clientId;
				ConPrint(outputPretext + L" client[" + std::to_wstring(clientId) + L"]\n");
				return true;
			}
		}
		else if (const auto& objectsByLabel = missions[parent.missionId].objectsByLabel.find(archetype->objNameOrLabel); objectsByLabel != missions[parent.missionId].objectsByLabel.end())
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