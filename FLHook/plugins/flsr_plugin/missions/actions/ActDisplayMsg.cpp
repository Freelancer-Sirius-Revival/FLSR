#include "ActDisplayMsg.h"

namespace Missions
{
	void ActDisplayMsg::Execute(Mission& mission, const MissionObject& activator) const
	{
		if (label == Activator)
		{
			if (activator.type == MissionObjectType::Client)
			{
				pub::Player::DisplayMissionMessage(activator.id, FmtStr(stringId, 0), MissionMessageType::MissionMessageType_Type1, true);
			}
		}
		else if (const auto& objectsByLabel = mission.objectsByLabel.find(label); objectsByLabel != mission.objectsByLabel.end())
		{
			for (const auto& object : objectsByLabel->second)
			{
				if (object.type == MissionObjectType::Client)
				{
					pub::Player::DisplayMissionMessage(object.id, FmtStr(stringId, 0), MissionMessageType::MissionMessageType_Type1, true);
				}
			}
		}
	}
}