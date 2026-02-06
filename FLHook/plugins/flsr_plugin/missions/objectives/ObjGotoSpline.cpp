#include "ObjGotoSpline.h"
#include "ObjCndDistVec.h"
#include "ObjUtils.h"

namespace Missions
{
	ObjGotoSpline::ObjGotoSpline(const ObjectiveParent& parent,
								const Vector splinePoints[4],
								const bool noCruise,
								const float range,
								const float thrust,
								const uint objNameToWaitFor,
								const float startWaitDistance,
								const float endWaitDistance) :
		Objective(parent),
		ObjGoto(noCruise, range, thrust, objNameToWaitFor, startWaitDistance, endWaitDistance)
	{
		spline[0] = splinePoints[0];
		spline[1] = splinePoints[1];
		spline[2] = splinePoints[2];
		spline[3] = splinePoints[3];
	}

	void ObjGotoSpline::Execute(const ObjectiveState& state) const
	{
		if (pub::SpaceObj::ExistsAndAlive(state.objId) != 0)
			return;

		Objective::Execute(state);

		pub::AI::DirectiveGotoOp gotoOp;
		gotoOp.fireWeapons = !state.enforceObjective;
		gotoOp.gotoType = pub::AI::GotoOpType::Spline;
		gotoOp.spline[0] = spline[0];
		gotoOp.spline[1] = spline[1];
		gotoOp.spline[2] = spline[2];
		gotoOp.spline[3] = spline[3];
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

		RegisterCondition(state.objId, ConditionPtr(new ObjCndDistVec(parent, state, range, spline[3])));
	}
}