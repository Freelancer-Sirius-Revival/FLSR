#include "ObjCndDistVec.h"

namespace Missions
{
	static uint GetSystemId(const uint objId)
	{
		uint systemId;
		pub::SpaceObj::GetSystem(objId, systemId);
		return systemId;
	}

	ObjCndDistVec::ObjCndDistVec(const ObjectiveParent& parent, const ObjectiveState& state, const float distance, const Vector& position) :
		CndDistVec(
			ConditionParent(parent.missionId, 0),
			missions.at(parent.missionId).FindObjNameByObjId(state.objId),
			CndDistVec::DistanceCondition::Inside,
			position,
			distance,
			GetSystemId(state.objId),
			""
		),
		parent(parent),
		state(state)
	{}

	void ObjCndDistVec::ExecuteTrigger()
	{
		Unregister();
		const auto& mission = missions.at(parent.missionId);
		const auto& objectives = mission.objectives.at(parent.objectivesId);
		ObjectiveState nextState(state);
		nextState.objectiveIndex++;
		objectives.Progress(nextState);
	}
}