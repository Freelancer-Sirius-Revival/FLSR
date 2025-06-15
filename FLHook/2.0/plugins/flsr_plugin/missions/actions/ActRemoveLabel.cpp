#include "ActRemoveLabel.h"

namespace Missions
{
	void ActRemoveLabel::Execute(Mission& mission, const MissionObject& activator) const
	{
		if (objNameOrLabel == Activator)
		{
			if (activator.type == MissionObjectType::Client)
				mission.RemoveLabelFromObject(activator, label);
			else if (mission.objectIds.contains(activator.id))
				mission.RemoveLabelFromObject(activator, label);
		}
		else
		{
			// Clients can only be addressed via Label.
			if (const auto& objectByName = mission.objectIdsByName.find(objNameOrLabel); objectByName != mission.objectIdsByName.end())
			{
				mission.RemoveLabelFromObject(MissionObject(MissionObjectType::Object, objectByName->second), label);
			}
			else if (const auto& objectsByLabel = mission.objectsByLabel.find(objNameOrLabel); objectsByLabel != mission.objectsByLabel.end())
			{
				const std::vector<MissionObject> objectsCopy(objectsByLabel->second);
				for (const auto& object : objectsCopy)
					mission.RemoveLabelFromObject(object, label);
			}
		}
	}
}