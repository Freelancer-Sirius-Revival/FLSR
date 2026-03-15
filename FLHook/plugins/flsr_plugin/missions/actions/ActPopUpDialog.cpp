#include "ActPopUpDialog.h"

namespace Missions
{
	void ActPopUpDialog::Execute(Mission& mission, const MissionObject& activator) const
	{
		if (label == Activator)
		{
			if (activator.type == MissionObjectType::Client)
			{
				pub::Player::PopUpDialog(activator.id, FmtStr(headingId, 0), FmtStr(messageId, 0), buttons);
			}
		}
		else if (const auto& objectsByLabel = mission.objectsByLabel.find(label); objectsByLabel != mission.objectsByLabel.end())
		{
			for (const auto& object : objectsByLabel->second)
			{
				if (object.type == MissionObjectType::Client)
				{
					pub::Player::PopUpDialog(object.id, FmtStr(headingId, 0), FmtStr(messageId, 0), buttons);
				}
			}
		}
	}
}