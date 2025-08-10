#include "ObjCndTimer.h"

namespace Missions
{
	ObjCndTimer::ObjCndTimer(const ObjectiveParent& parent, const ObjectiveState& state, const float time) :
		CndTimer(ConditionParent(parent.missionId, 0), time, 0.0f),
		parent(parent),
		state(state)
	{}

	void ObjCndTimer::ExecuteTrigger()
	{
		Unregister();
		const auto& mission = missions.at(parent.missionId);
		const auto& objectives = mission.objectives.at(parent.objectivesId);
		ObjectiveState nextState(state);
		nextState.objectiveIndex++;
		objectives.Progress(nextState);
	}
}