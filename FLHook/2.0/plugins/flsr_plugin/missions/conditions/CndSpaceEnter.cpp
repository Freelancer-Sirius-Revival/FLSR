#include "CndSpaceEnter.h"
#include "../Mission.h"

namespace Missions
{
	std::unordered_set<CndSpaceEnter*> spaceEnterConditions;

	CndSpaceEnter::CndSpaceEnter(const ConditionParent& parent, const CndSpaceEnterArchetypePtr conditionArchetype) :
		Condition(parent, ConditionType::Cnd_SpaceEnter),
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
		auto& mission = missions.at(parent.missionId);
		auto& trigger = mission.triggers.at(parent.triggerId);
		const std::wstring outputPretext = stows(mission.archetype->name) + L"->" + stows(trigger.archetype->name) + L": Cnd_SpaceEnter " + std::to_wstring(archetype->objNameOrLabel) + L" into " + std::to_wstring(archetype->systemId);

		if (archetype->objNameOrLabel == Stranger)
		{
			if (!mission.clientIds.contains(clientId) && (!archetype->systemId || archetype->systemId == systemId))
			{
				trigger.activator.type = MissionObjectType::Client;
				trigger.activator.id = clientId;
				ConPrint(outputPretext + L" client[" + std::to_wstring(clientId) + L"]\n");
				return true;
			}
		}
		else if (const auto& objectsByLabel = mission.objectsByLabel.find(archetype->objNameOrLabel); objectsByLabel != mission.objectsByLabel.end())
		{
			for (const auto& object : objectsByLabel->second)
			{
				if (object.type == MissionObjectType::Client && object.id == clientId && (!archetype->systemId || archetype->systemId == systemId))
				{
					trigger.activator = object;
					ConPrint(outputPretext + L" client[" + std::to_wstring(object.id) + L"]\n");
					return true;
				}
			}
		}
		return false;
	}
}