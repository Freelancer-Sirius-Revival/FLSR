#include "ObjDelay.h"
#include "ObjCndTimer.h"

namespace Missions
{
	ObjDelay::ObjDelay(const ObjectiveParent& parent, const float timeInS) :
		Objective(parent),
		timeInS(timeInS)
	{}

	void ObjDelay::Execute(const ObjectiveState& state) const
	{
		if (pub::SpaceObj::ExistsAndAlive(state.objId) != 0)
			return;

		Objective::Execute(state);
		
		pub::AI::DirectiveDelayOp delayOp;
		delayOp.fireWeapons = !state.enforceObjective;
		delayOp.DelayTime = timeInS;
		pub::AI::SubmitDirective(state.objId, &delayOp);

		RegisterCondition(state.objId, ConditionPtr(new ObjCndTimer(parent, state, timeInS)));
	}
}