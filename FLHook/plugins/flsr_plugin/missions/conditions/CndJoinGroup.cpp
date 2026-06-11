#include "CndJoinGroup.h"
#include "../Mission.h"
#include "../../Plugin.h"

namespace Missions
{
	std::unordered_set<CndJoinGroup*> observedCndJoinGroup;
	std::vector<CndJoinGroup*> orderedCndJoinGroup;

	CndJoinGroup::CndJoinGroup(const ConditionParent& parent, const uint label) :
		Condition(parent),
		label(label)
	{}

	CndJoinGroup::~CndJoinGroup()
	{
		Unregister();
	}

	void CndJoinGroup::Register()
	{
		if (observedCndJoinGroup.insert(this).second)
			orderedCndJoinGroup.push_back(this);
	}

	void CndJoinGroup::Unregister()
	{
		observedCndJoinGroup.erase(this);
		if (const auto it = std::find(orderedCndJoinGroup.begin(), orderedCndJoinGroup.end(), this); it != orderedCndJoinGroup.end())
			orderedCndJoinGroup.erase(it);
	}

	bool CndJoinGroup::Matches(const uint clientId, CPlayerGroup& group)
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
		namespace CndJoinGroup
		{
			bool __stdcall AddGroupMemberHook_After(CPlayerGroup* group, uint clientId)
			{
				const auto currentConditions(orderedCndJoinGroup);
				for (const auto& condition : currentConditions)
				{
					if (observedCndJoinGroup.contains(condition) && condition->Matches(clientId, *group))
						condition->ExecuteTrigger();
				}
				returncode = DEFAULT_RETURNCODE;
				return true;
			}
		}
	}
}