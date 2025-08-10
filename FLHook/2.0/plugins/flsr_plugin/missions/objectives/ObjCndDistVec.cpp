#include "ObjCndDistVec.h"

namespace Missions
{
	static uint GetSystemId(const uint objId)
	{
		uint systemId;
		pub::SpaceObj::GetSystem(objId, systemId);
		return systemId;
	}

	ObjCndDistVec::ObjCndDistVec(const ObjectiveParent& parent, const int objectiveIndex, const uint objId, const float distance, const Vector& position) :
		CndDistVec(ConditionParent(parent.missionId, 0), missions.at(parent.missionId).FindObjNameByObjId(objId), CndDistVec::DistanceCondition::Inside, position, distance, GetSystemId(objId)),
		parent(parent),
		objectiveIndex(objectiveIndex),
		objId(objId)
	{}

	void ObjCndDistVec::ExecuteTrigger()
	{
		Unregister();
		const auto& mission = missions.at(parent.missionId);
		const auto& objectives = mission.objectives.at(parent.objectivesId);
		objectives.Progress(objId, objectiveIndex + 1);
	}
}