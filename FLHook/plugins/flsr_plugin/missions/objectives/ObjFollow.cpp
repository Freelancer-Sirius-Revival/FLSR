#include "ObjFollow.h"
#include "ObjCndTrue.h"
#include "ObjUtils.h"

namespace Missions
{
	ObjFollow::ObjFollow(const ObjectiveParent& parent, const uint targetObjName, const float maxDistance, const Vector& position) :
		Objective(parent),
		targetObjName(targetObjName),
		maxDistance(maxDistance),
		position(position)
	{}

	void ObjFollow::Execute(const ObjectiveState& state) const
	{
		if (pub::SpaceObj::ExistsAndAlive(state.objId) != 0)
			return;

		Objective::Execute(state);

		if (uint followTargetSpaceObjId; FindObjectByNameOrFirstPlayerByLabel(missions.at(parent.missionId), targetObjName, followTargetSpaceObjId))
		{
			pub::AI::DirectiveFollowOp followOp;
			followOp.fireWeapons = !state.enforceObjective;
			followOp.followSpaceObj = followTargetSpaceObjId;
			followOp.maxDistance = maxDistance;
			followOp.offset = position;
			followOp.dunno2 = 400.0f;
			pub::AI::SubmitDirective(state.objId, &followOp);
		}

		RegisterCondition(state.objId, ConditionPtr(new ObjCndTrue(parent, state)));
	}
}