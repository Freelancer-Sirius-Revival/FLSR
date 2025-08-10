#include "ObjCndDistObj.h"

namespace Missions
{
	ObjCndDistObj::ObjCndDistObj(const ObjectiveParent& parent, const int objectiveIndex, const uint objId, const float distance, const uint otherObjNameOrLabel) :
		CndDistObj(ConditionParent(parent.missionId, 0), missions.at(parent.missionId).FindObjNameByObjId(objId), CndDistObj::DistanceCondition::Inside, distance, otherObjNameOrLabel),
		parent(parent),
		objectiveIndex(objectiveIndex),
		objId(objId)
	{}

	void ObjCndDistObj::ExecuteTrigger()
	{
		Unregister();
		const auto& mission = missions.at(parent.missionId);
		const auto& objectives = mission.objectives.at(parent.objectivesId);
		objectives.Progress(objId, objectiveIndex + 1);
	}
}