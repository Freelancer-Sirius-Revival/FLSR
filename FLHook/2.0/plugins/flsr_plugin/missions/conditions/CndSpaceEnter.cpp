#include "CndSpaceEnter.h"
#include "../Mission.h"

namespace Missions
{
	std::unordered_set<CndSpaceEnter*> spaceEnterConditions;

	CndSpaceEnter::CndSpaceEnter(const ConditionParent& parent, const uint objNameOrLabel, const uint systemId) :
		Condition(parent),
		objNameOrLabel(objNameOrLabel),
		systemId(systemId)
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

	bool CndSpaceEnter::Matches(const uint clientId, const uint currentSystemId)
	{
		const auto& mission = missions.at(parent.missionId);
		if (objNameOrLabel == Stranger)
		{
			if (!mission.clientIds.contains(clientId) && (!systemId || systemId == currentSystemId))
			{
				activator.type = MissionObjectType::Client;
				activator.id = clientId;
				return true;
			}
		}
		else if (const auto& objectsByLabel = mission.objectsByLabel.find(objNameOrLabel); objectsByLabel != mission.objectsByLabel.end())
		{
			for (const auto& object : objectsByLabel->second)
			{
				if (object.type == MissionObjectType::Client && object.id == clientId && (!systemId || systemId == currentSystemId))
				{
					activator = object;
					return true;
				}
			}
		}
		return false;
	}
}