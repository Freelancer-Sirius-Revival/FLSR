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
		auto& mission = missions.at(parent.missionId);
		const auto& objectives = mission.objectives.at(parent.objectivesId);
		const int nextObjectiveIndex = objectiveIndex + 1;
		if (nextObjectiveIndex < objectives.objectives.size())
			objectives.objectives[nextObjectiveIndex].Execute(objId);

		mission.dynamicConditions.erase(this);
	}
}