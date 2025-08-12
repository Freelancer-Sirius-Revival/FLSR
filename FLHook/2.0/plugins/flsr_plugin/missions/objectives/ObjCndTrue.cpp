#include "ObjCndTrue.h"

namespace Missions
{
	ObjCndTrue::ObjCndTrue(const ObjectiveParent& parent, const ObjectiveState& state) :
		CndTrue(ConditionParent(parent.missionId, 0)),
		parent(parent),
		state(state)
	{}

	void ObjCndTrue::ExecuteTrigger()
	{
		Unregister();
		const auto& mission = missions.at(parent.missionId);
		const auto& objectives = mission.objectives.at(parent.objectivesId);
		ObjectiveState nextState(state);
		nextState.objectiveIndex++;
		objectives.Progress(nextState);
	}
}