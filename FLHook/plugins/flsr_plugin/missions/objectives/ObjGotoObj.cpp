#include "ObjGotoObj.h"
#include "ObjCndDistObj.h"
#include "ObjCndTrue.h"
#include "ObjUtils.h"

namespace Missions
{
	ObjGotoObj::ObjGotoObj(const ObjectiveParent& parent,
							const uint targetObjNameOrId,
							const bool noCruise,
							const float range,
							const float thrust,
							const uint objNameToWaitFor,
							const float startWaitDistance,
							const float endWaitDistance) :
		Objective(parent),
		ObjGoto(noCruise, range, thrust, objNameToWaitFor, startWaitDistance, endWaitDistance),
		targetObjNameOrId(targetObjNameOrId)
	{}

	void ObjGotoObj::Execute(const ObjectiveState& state) const
	{
		if (pub::SpaceObj::ExistsAndAlive(state.objId) != 0)
			return;

		Objective::Execute(state);

		const auto& mission = missions.at(parent.missionId);
		uint targetObjId = 0;
		if (!FindObjectByNameOrFirstPlayerByLabel(mission, targetObjNameOrId, targetObjId))
		{
			// Assume the target to be a static world object.
			if (pub::SpaceObj::ExistsAndAlive(targetObjNameOrId) == 0)
				targetObjId = targetObjNameOrId;
		}

		if (targetObjId)
		{
			pub::AI::DirectiveGotoOp gotoOp;
			gotoOp.fireWeapons = !state.enforceObjective;
			gotoOp.gotoType = pub::AI::GotoOpType::Ship;
			gotoOp.targetId = targetObjNameOrId;
			gotoOp.range = range;
			gotoOp.thrust = thrust;
			gotoOp.shipMoves = true;
			gotoOp.shipMoves2 = true;
			gotoOp.goToCruise = !noCruise;
			gotoOp.goToNoCruise = noCruise;
			if (uint waitForObjId; FindObjectByNameOrFirstPlayerByLabel(mission, objNameToWaitFor, waitForObjId))
				gotoOp.objIdToWaitFor = waitForObjId;
			else
				gotoOp.objIdToWaitFor = 0;
			gotoOp.startWaitDistance = startWaitDistance;
			gotoOp.endWaitDistance = endWaitDistance;
			pub::AI::SubmitDirective(state.objId, &gotoOp);

			RegisterCondition(state.objId, ConditionPtr(new ObjCndDistObj(parent, state, range, targetObjNameOrId)));
		}
		else
		{
			RegisterCondition(state.objId, ConditionPtr(new ObjCndTrue(parent, state)));
		}
	}
}