#include "CndLeaveGroup.h"
#include "../Mission.h"
#include "../../Plugin.h"

namespace Missions
{
	std::unordered_set<CndLeaveGroup*> observedCndLeaveGroup;
	std::vector<CndLeaveGroup*> orderedCndLeaveGroup;

	CndLeaveGroup::CndLeaveGroup(const ConditionParent& parent, const uint label) :
		Condition(parent),
		label(label)
	{}

	CndLeaveGroup::~CndLeaveGroup()
	{
		Unregister();
	}

	void CndLeaveGroup::Register()
	{
		if (observedCndLeaveGroup.insert(this).second)
			orderedCndLeaveGroup.push_back(this);
	}

	void CndLeaveGroup::Unregister()
	{
		observedCndLeaveGroup.erase(this);
		if (const auto it = std::find(orderedCndLeaveGroup.begin(), orderedCndLeaveGroup.end(), this); it != orderedCndLeaveGroup.end())
			orderedCndLeaveGroup.erase(it);
	}

	bool CndLeaveGroup::Matches(const uint clientId, CPlayerGroup& group)
	{
		const auto& mission = missions.at(parent.missionId);
		if (!label)
		{
			for (const uint otherClientId : mission.clientIds)
			{
				if (otherClientId != clientId && group.IsMember(otherClientId))
				{
					activator = MissionObject(MissionObjectType::Client, clientId);
					return true;
				}
			}
		}
		else if (const auto& objectsByLabel = mission.objectsByLabel.find(label); objectsByLabel != mission.objectsByLabel.end())
		{
			for (const auto& object : objectsByLabel->second)
			{
				if (object.type == MissionObjectType::Client && object.id != clientId && group.IsMember(object.id))
				{
					activator = MissionObject(MissionObjectType::Client, clientId);
					return true;
				}
			}
		}
		return false;
	}

	namespace Hooks
	{
		namespace CndLeaveGroup
		{
			bool __stdcall DelGroupMemberHook(CPlayerGroup* group, uint clientId)
			{
				const auto currentConditions(orderedCndLeaveGroup);
				for (const auto& condition : currentConditions)
				{
					if (observedCndLeaveGroup.contains(condition) && condition->Matches(clientId, *group))
						condition->ExecuteTrigger();
				}
				returncode = DEFAULT_RETURNCODE;
				return true;
			}
		}
	}
}