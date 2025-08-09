#include "ObjDelay.h"
#include "ObjCndTimer.h"
#include "../Mission.h"

namespace Missions
{
	ObjDelay::ObjDelay(const ObjectiveParent& parent, const int objectiveIndex, const float timeInS) :
		Objective(parent, objectiveIndex),
		timeInS(timeInS)
	{}

	void ObjDelay::Execute(const uint objId) const
	{
		if (pub::SpaceObj::ExistsAndAlive(objId) != 0)
			return;

		Objective::Execute(objId);
		
		pub::AI::DirectiveDelayOp delayOp;
		delayOp.fireWeapons = false;
		delayOp.DelayTime = timeInS;
		pub::AI::SubmitDirective(objId, &delayOp);

		RegisterCondition(objId, ConditionPtr(new ObjCndTimer(parent, objectiveIndex, objId, timeInS)));
	}
}