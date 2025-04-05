#include "CndSpaceExit.h"
#include "../Mission.h"

namespace Missions
{
	std::unordered_set<CndSpaceExit*> spaceExitConditions;

	CndSpaceExit::CndSpaceExit(const ConditionParent& parent, const CndSpaceExitArchetypePtr conditionArchetype) :
		Condition(parent, ConditionType::Cnd_SpaceExit),
		archetype(conditionArchetype)
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