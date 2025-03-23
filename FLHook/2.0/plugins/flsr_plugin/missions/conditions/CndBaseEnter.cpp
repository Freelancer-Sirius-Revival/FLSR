#include "CndBaseEnter.h"
#include "../Mission.h"

namespace Missions
{
	std::unordered_set<CndBaseEnter*> baseEnterConditions;

	CndBaseEnter::CndBaseEnter(const ConditionParent& parent, const CndBaseEnterArchetypePtr conditionArchetype) :
		Condition(parent, ConditionType::Cnd_BaseEnter),
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
		auto& mission = missions.at(parent.missionId);
		auto& trigger = mission.triggers.at(parent.triggerId);
		const std::wstring outputPretext = stows(mission.archetype->name) + L"->" + stows(trigger.archetype->name) + L": Cnd_BaseEnter " + std::to_wstring(archetype->objNameOrLabel) + L" on " + std::to_wstring(archetype->baseId);

		if (archetype->objNameOrLabel == Stranger)
		{
			if (!mission.clientIds.contains(clientId) && (!archetype->baseId || archetype->baseId == baseId))
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
				if (object.type == MissionObjectType::Client && object.id == clientId && (!archetype->baseId || archetype->baseId == baseId))
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