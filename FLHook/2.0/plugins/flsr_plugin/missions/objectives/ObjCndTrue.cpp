#include "ObjCndTrue.h"

namespace Missions
{
	ObjCndTrue::ObjCndTrue(const ObjectiveParent& parent, const int objectiveIndex, const uint objId) :
		CndTrue(ConditionParent(parent.missionId, 0)),
		parent(parent),
		objectiveIndex(objectiveIndex),
		objId(objId)
	{}

	void ObjCndTrue::ExecuteTrigger()
	{
		Unregister();
		const auto& mission = missions.at(parent.missionId);
		const auto& objectives = mission.objectives.at(parent.objectivesId);
		objectives.Progress(objId, objectiveIndex + 1);
	}
}