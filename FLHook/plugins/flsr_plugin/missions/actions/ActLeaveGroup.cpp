#include "ActLeaveGroup.h"

namespace Missions
{
	static void LeaveGroup(const uint clientId)
	{
		const uint groupId = Players.GetGroupID(clientId);
		if (!groupId)
			return;

		CPlayerGroup* group = CPlayerGroup::FromGroupID(groupId);
		if (group)
			group->DelMember(clientId);
	}

	void ActLeaveGroup::Execute(Mission& mission, const MissionObject& activator) const
	{
		if (label == Activator)
		{
			if (activator.type == MissionObjectType::Client)
			{
				LeaveGroup(activator.id);
			}
		}
		else if (const auto& objectsByLabel = mission.objectsByLabel.find(label); objectsByLabel != mission.objectsByLabel.end())
		{
			for (const auto& object : objectsByLabel->second)
			{
				if (object.type == MissionObjectType::Client)
				{
					LeaveGroup(object.id);
				}
			}
		}
	}
}