#include "ObjIdle.h"
#include "ObjCndTrue.h"

namespace Missions
{
	ObjIdle::ObjIdle(const ObjectiveParent& parent, const int objectiveIndex) : Objective(parent, objectiveIndex)
	{}

	void ObjIdle::Execute(const uint objId) const
	{
		if (pub::SpaceObj::ExistsAndAlive(objId) != 0)
			return;

		Objective::Execute(objId);

		pub::AI::DirectiveIdleOp idleOp;
		idleOp.fireWeapons = false;
		pub::AI::SubmitDirective(objId, &idleOp);

		RegisterCondition(objId, ConditionPtr(new ObjCndTrue(parent, objectiveIndex, objId)));
	}
}