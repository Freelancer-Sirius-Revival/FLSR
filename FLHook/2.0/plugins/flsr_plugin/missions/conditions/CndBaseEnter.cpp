#include "CndBaseEnter.h"
#include "../Mission.h"

namespace Missions
{
	std::unordered_set<CndBaseEnter*> baseEnterConditions;

	CndBaseEnter::CndBaseEnter(const ConditionParent& parent, const CndBaseEnterArchetypePtr conditionArchetype) :
		Condition(parent, ConditionType::Cnd_BaseEnter),
		archetype(conditionArchetype)
	{}

	CndBaseEnter::~CndBaseEnter()
	{
		Unregister();
	}

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
		if (archetype->objNameOrLabel == Stranger)
		{
			if (!mission.clientIds.contains(clientId) && (!archetype->baseId || archetype->baseId == baseId))
			{
				activator.type = MissionObjectType::Client;
				activator.id = clientId;
				return true;
			}
		}
		else if (const auto& objectsByLabel = mission.objectsByLabel.find(archetype->objNameOrLabel); objectsByLabel != mission.objectsByLabel.end())
		{
			for (const auto& object : objectsByLabel->second)
			{
				if (object.type == MissionObjectType::Client && object.id == clientId && (!archetype->baseId || archetype->baseId == baseId))
				{
					activator = object;
					return true;
				}
			}
		}
		return false;
	}
}