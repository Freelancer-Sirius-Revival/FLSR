#include "ObjFollow.h"
#include "ObjCndTrue.h"
#include "../Mission.h"

namespace Missions
{
	ObjFollow::ObjFollow(const ObjectiveParent& parent, const int objectiveIndex, const uint targetObjName, const float maxDistance, const Vector& position) :
		Objective(parent, objectiveIndex),
		targetObjName(targetObjName),
		maxDistance(maxDistance),
		position(position)
	{}

	void ObjFollow::Execute(const uint objId) const
	{
		if (pub::SpaceObj::ExistsAndAlive(objId) != 0)
			return;

		Objective::Execute(objId);

		auto& mission = missions.at(parent.missionId);
		const auto& objectEntry = mission.objectIdsByName.find(targetObjName);
		if (objectEntry == mission.objectIdsByName.end())
			return;
		pub::AI::DirectiveFollowOp followOp;
		followOp.fireWeapons = false;
		followOp.followSpaceObj = objectEntry->second;
		followOp.maxDistance = maxDistance;
		followOp.offset = position;
		followOp.dunno2 = 400.0f;
		pub::AI::SubmitDirective(objId, &followOp);

		RegisterCondition(objId, ConditionPtr(new ObjCndTrue(parent, objectiveIndex, objId)));
	}
}