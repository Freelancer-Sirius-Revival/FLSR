#include "ObjIdle.h"
#include "ObjCndTrue.h"

namespace Missions
{
	ObjIdle::ObjIdle(const ObjectiveParent& parent) : Objective(parent)
	{}

	void ObjIdle::Execute(const ObjectiveState& state) const
	{
		if (pub::SpaceObj::ExistsAndAlive(state.objId) != 0)
			return;

		Objective::Execute(state);

		pub::AI::DirectiveIdleOp idleOp;
		idleOp.fireWeapons = true;
		pub::AI::SubmitDirective(state.objId, &idleOp);

		RegisterCondition(state.objId, ConditionPtr(new ObjCndTrue(parent, state)));
	}
}