#include "ObjGotoVec.h"
#include "ObjCndDistVec.h"
#include "../Mission.h"

namespace Missions
{
	ObjGotoVec::ObjGotoVec(const ObjectiveParent& parent,
							const int objectiveIndex,
							const Vector& position,
							const bool noCruise,
							const float range,
							const float thrust,
							const uint objNameToWaitFor,
							const float startWaitDistance,
							const float endWaitDistance) :
		Objective(parent, objectiveIndex),
		ObjGoto(noCruise, range, thrust, objNameToWaitFor, startWaitDistance, endWaitDistance),
		position(position)
	{}

	void ObjGotoVec::Execute(const uint objId) const
	{
		if (pub::SpaceObj::ExistsAndAlive(objId) != 0)
			return;

		Objective::Execute(objId);

		pub::AI::DirectiveGotoOp gotoOp;
		gotoOp.fireWeapons = false;
		gotoOp.gotoType = pub::AI::GotoOpType::Vec;
		gotoOp.pos = position;
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

		RegisterCondition(objId, ConditionPtr(new ObjCndDistVec(parent, objectiveIndex, objId, range, position)));
	}
}