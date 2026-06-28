#include "CndTrue.h"
#include "../Mission.h"

namespace Missions
{
	CndTrue::CndTrue(const ConditionParent& parent, const uint activatorLabel) :
		Condition(parent),
		activatorLabel(activatorLabel)
	{}

	ConditionPtr CndTrue::Copy(const ConditionParent& newParent, const uint overrideObjNameOrLabel) const
	{
		return ConditionPtr(new CndTrue(newParent, overrideObjNameOrLabel));
	}

	void CndTrue::Register()
	{
		activator = MissionObject(MissionObjectType::Client, 0);
		if (activatorLabel != 0)
		{
			const auto& mission = missions.at(parent.missionId);
			const auto& objectsByLabel = mission.objectsByLabel.find(activatorLabel);
			if (objectsByLabel != mission.objectsByLabel.end() && !objectsByLabel->second.empty())
				activator = objectsByLabel->second.at(0);
		}
		ExecuteTrigger();
	}
}