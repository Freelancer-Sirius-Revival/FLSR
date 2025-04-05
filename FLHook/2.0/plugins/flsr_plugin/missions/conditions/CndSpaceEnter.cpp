#include "CndSpaceEnter.h"
#include "../Mission.h"

namespace Missions
{
	std::unordered_set<CndSpaceEnter*> spaceEnterConditions;

	CndSpaceEnter::CndSpaceEnter(const ConditionParent& parent, const CndSpaceEnterArchetypePtr conditionArchetype) :
		Condition(parent, ConditionType::Cnd_SpaceEnter),
		archetype(conditionArchetype)
	{}

	CndSpaceEnter::~CndSpaceEnter()
	{
		Unregister();
	}

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
		if (archetype->objNameOrLabel == Stranger)
		{
			if (!mission.clientIds.contains(clientId) && (!archetype->systemId || archetype->systemId == systemId))
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
				if (object.type == MissionObjectType::Client && object.id == clientId && (!archetype->systemId || archetype->systemId == systemId))
				{
					activator = object;
					return true;
				}
			}
		}
		return false;
	}
}