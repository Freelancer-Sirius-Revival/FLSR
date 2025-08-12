#include "ObjGotoObj.h"
#include "ObjCndDistObj.h"
#include "ObjCndTrue.h"

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

		uint targetObjId = 0;
		const auto& mission = missions.at(parent.missionId);
		if (const auto& objectEntry = mission.objectIdsByName.find(targetObjNameOrId); objectEntry != mission.objectIdsByName.end())
			targetObjId = objectEntry->second;
		if (!targetObjId)
			targetObjId = targetObjNameOrId;

		if (pub::SpaceObj::ExistsAndAlive(targetObjId) == 0)
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
			gotoOp.objIdToWaitFor = 0;
			if (const auto& objectEntry = mission.objectIdsByName.find(objNameToWaitFor); objectEntry != mission.objectIdsByName.end())
				gotoOp.objIdToWaitFor = objectEntry->second;
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