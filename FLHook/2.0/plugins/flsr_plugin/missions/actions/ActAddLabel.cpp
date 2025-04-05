#include "ActAddLabel.h"

namespace Missions
{
	void ActAddLabel::Execute(Mission& mission, const MissionObject& activator) const
	{
		if (objNameOrLabel == Activator)
		{
			if (activator.type == MissionObjectType::Client)
			{
				mission.AddLabelToObject(activator, label);
			}
			else if (mission.objectIds.contains(activator.id))
			{
				mission.AddLabelToObject(activator, label);
			}
		}
		else
		{
			// Clients can only be addressed via Label.
			if (const auto& objectByName = mission.objectIdsByName.find(objNameOrLabel); objectByName != mission.objectIdsByName.end())
			{
				MissionObject object;
				object.type = MissionObjectType::Object;
				object.id = objectByName->second;
				mission.AddLabelToObject(object, label);
			}
			else if (const auto& objectsByLabel = mission.objectsByLabel.find(objNameOrLabel); objectsByLabel != mission.objectsByLabel.end())
			{
				for (const auto& object : objectsByLabel->second)
					mission.AddLabelToObject(object, label);
			}
		}
	}
}