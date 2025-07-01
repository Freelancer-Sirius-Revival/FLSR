#include "CndSpaceExit.h"
#include "../Mission.h"

namespace Missions
{
	std::unordered_set<CndSpaceExit*> spaceExitConditions;

	CndSpaceExit::CndSpaceExit(const ConditionParent& parent, const uint objNameOrLabel, const uint systemId) :
		Condition(parent),
		objNameOrLabel(objNameOrLabel),
		systemId(systemId)
	{}

	CndSpaceExit::~CndSpaceExit()
	{
		Unregister();
	}

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
		const auto& mission = missions.at(parent.missionId);
		if (objNameOrLabel == Stranger)
		{
			if (!mission.clientIds.contains(clientId) && (!systemId || systemId == systemId))
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
				if (object.type == MissionObjectType::Client && object.id == clientId && (!systemId || systemId == systemId))
				{
					activator = object;
					return true;
				}
			}
		}
		return false;
	}
}