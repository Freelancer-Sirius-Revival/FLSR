#include "ObjCndDistObj.h"

namespace Missions
{
	ObjCndDistObj::ObjCndDistObj(const ObjectiveParent& parent, const ObjectiveState& state, const float distance, const uint otherObjNameOrLabel) :
		CndDistObj(ConditionParent(parent.missionId, 0), missions.at(parent.missionId).FindObjNameByObjId(state.objId), CndDistObj::DistanceCondition::Inside, distance, otherObjNameOrLabel),
		parent(parent),
		state(state)
	{}

	void ObjCndDistObj::ExecuteTrigger()
	{
		Unregister();
		const auto& mission = missions.at(parent.missionId);
		const auto& objectives = mission.objectives.at(parent.objectivesId);
		ObjectiveState nextState(state);
		nextState.objectiveIndex++;
		objectives.Progress(nextState);
	}
}