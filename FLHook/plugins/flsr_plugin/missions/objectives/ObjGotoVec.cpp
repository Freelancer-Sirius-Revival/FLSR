#include "ObjGotoVec.h"
#include "ObjCndDistVec.h"
#include "ObjUtils.h"

namespace Missions
{
	ObjGotoVec::ObjGotoVec(const ObjectiveParent& parent,
							const Vector& position,
							const bool noCruise,
							const float range,
							const float thrust,
							const uint objNameToWaitFor,
							const float startWaitDistance,
							const float endWaitDistance) :
		Objective(parent),
		ObjGoto(noCruise, range, thrust, objNameToWaitFor, startWaitDistance, endWaitDistance),
		position(position)
	{}

	void ObjGotoVec::Execute(const ObjectiveState& state) const
	{
		if (pub::SpaceObj::ExistsAndAlive(state.objId) != 0)
			return;

		Objective::Execute(state);

		pub::AI::DirectiveGotoOp gotoOp;
		gotoOp.fireWeapons = !state.enforceObjective;
		gotoOp.gotoType = pub::AI::GotoOpType::Vec;
		gotoOp.pos = position;
		gotoOp.range = range;
		gotoOp.thrust = thrust;
		gotoOp.shipMoves = true;
		gotoOp.shipMoves2 = true;
		gotoOp.goToCruise = !noCruise;
		gotoOp.goToNoCruise = noCruise;
		if (uint waitForObjId; FindObjectByNameOrFirstPlayerByLabel(missions.at(parent.missionId), objNameToWaitFor, waitForObjId))
			gotoOp.objIdToWaitFor = waitForObjId;
		else
			gotoOp.objIdToWaitFor = 0;
		gotoOp.startWaitDistance = startWaitDistance;
		gotoOp.endWaitDistance = endWaitDistance;
		pub::AI::SubmitDirective(state.objId, &gotoOp);

		RegisterCondition(state.objId, ConditionPtr(new ObjCndDistVec(parent, state, range, position)));
	}
}