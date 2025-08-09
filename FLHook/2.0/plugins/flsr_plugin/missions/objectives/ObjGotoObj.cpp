#include "ObjGotoObj.h"
#include "ObjCndDistObj.h"
#include "../Mission.h"

namespace Missions
{
	ObjGotoObj::ObjGotoObj(const ObjectiveParent& parent,
							const int objectiveIndex,
							const uint targetObjNameOrId,
							const bool noCruise,
							const float range,
							const float thrust,
							const uint objNameToWaitFor,
							const float startWaitDistance,
							const float endWaitDistance) :
		Objective(parent, objectiveIndex),
		ObjGoto(noCruise, range, thrust, objNameToWaitFor, startWaitDistance, endWaitDistance),
		targetObjNameOrId(targetObjNameOrId)
	{}

	void ObjGotoObj::Execute(const uint objId) const
	{
		if (pub::SpaceObj::ExistsAndAlive(objId) != 0)
			return;

		Objective::Execute(objId);

		const auto& mission = missions.at(parent.missionId);
		pub::AI::DirectiveGotoOp gotoOp;
		gotoOp.fireWeapons = false;
		gotoOp.gotoType = pub::AI::GotoOpType::Vec;
		gotoOp.targetId = 0;
		if (const auto& objectEntry = mission.objectIdsByName.find(targetObjNameOrId); objectEntry != mission.objectIdsByName.end())
			gotoOp.targetId = objectEntry->second;
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
		pub::AI::SubmitDirective(objId, &gotoOp);

		RegisterCondition(objId, ConditionPtr(new ObjCndDistObj(parent, objectiveIndex, objId, range, targetObjNameOrId)));
	}
}