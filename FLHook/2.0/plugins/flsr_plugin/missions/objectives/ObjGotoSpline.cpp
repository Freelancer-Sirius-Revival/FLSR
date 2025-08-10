#include "ObjGotoSpline.h"
#include "ObjCndDistVec.h"

namespace Missions
{
	ObjGotoSpline::ObjGotoSpline(const ObjectiveParent& parent,
								const int objectiveIndex,
								const Vector splinePoints[4],
								const bool noCruise,
								const float range,
								const float thrust,
								const uint objNameToWaitFor,
								const float startWaitDistance,
								const float endWaitDistance) :
		Objective(parent, objectiveIndex),
		ObjGoto(noCruise, range, thrust, objNameToWaitFor, startWaitDistance, endWaitDistance)
	{
		spline[0] = splinePoints[0];
		spline[1] = splinePoints[1];
		spline[2] = splinePoints[2];
		spline[3] = splinePoints[3];
	}

	void ObjGotoSpline::Execute(const uint objId) const
	{
		if (pub::SpaceObj::ExistsAndAlive(objId) != 0)
			return;

		Objective::Execute(objId);

		pub::AI::DirectiveGotoOp gotoOp;
		gotoOp.fireWeapons = false;
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
		gotoOp.objIdToWaitFor = 0;
		const auto& mission = missions.at(parent.missionId);
		if (const auto& objectEntry = mission.objectIdsByName.find(objNameToWaitFor); objectEntry != mission.objectIdsByName.end())
			gotoOp.objIdToWaitFor = objectEntry->second;
		gotoOp.startWaitDistance = startWaitDistance;
		gotoOp.endWaitDistance = endWaitDistance;
		pub::AI::SubmitDirective(objId, &gotoOp);

		RegisterCondition(objId, ConditionPtr(new ObjCndDistVec(parent, objectiveIndex, objId, range, spline[3])));
	}
}