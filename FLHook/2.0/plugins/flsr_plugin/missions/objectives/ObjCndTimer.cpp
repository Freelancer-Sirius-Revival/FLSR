#include "ObjCndTimer.h"

namespace Missions
{
	ObjCndTimer::ObjCndTimer(const ObjectiveParent& parent, const int objectiveIndex, const uint objId, const float time) :
		CndTimer(ConditionParent(parent.missionId, 0), time, 0.0f),
		parent(parent),
		objectiveIndex(objectiveIndex),
		objId(objId)
	{}

	void ObjCndTimer::ExecuteTrigger()
	{
		Unregister();
		const auto& mission = missions.at(parent.missionId);
		const auto& objectives = mission.objectives.at(parent.objectivesId);
		objectives.Progress(objId, objectiveIndex + 1);
	}
}