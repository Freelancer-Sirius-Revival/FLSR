#include "ObjDock.h"
#include "ObjCndTrue.h"

namespace Missions
{
	ObjDock::ObjDock(const ObjectiveParent& parent, const uint targetObjNameOrId) :
		Objective(parent),
		targetObjNameOrId(targetObjNameOrId)
	{}

	void ObjDock::Execute(const ObjectiveState& state) const
	{
		if (pub::SpaceObj::ExistsAndAlive(state.objId) != 0)
			return;

		Objective::Execute(state);

		uint dockTargetObjId = 0;
		const auto& mission = missions.at(parent.missionId);
		if (const auto& objectEntry = mission.objectIdsByName.find(targetObjNameOrId); objectEntry != mission.objectIdsByName.end())
			dockTargetObjId = objectEntry->second;
		if (!dockTargetObjId)
			dockTargetObjId = targetObjNameOrId;

		if (pub::SpaceObj::ExistsAndAlive(dockTargetObjId) == 0)
		{
			pub::AI::DirectiveDockOp dockOp;
			dockOp.fireWeapons = !state.enforceObjective;
			dockOp.dockTargetObjId = dockTargetObjId;
			dockOp.dockTargetDirectionObjId = 0;
			dockOp.x12 = 0;
			dockOp.dockPortIndex = -1;
			dockOp.x1C = 0;
			dockOp.x20 = 200.0f;
			dockOp.x24 = 500.0f;
			dockOp.x28 = 0;
			pub::AI::SubmitDirective(state.objId, &dockOp);
		}

		if (parent.objectivesId)
			RegisterCondition(state.objId, ConditionPtr(new ObjCndTrue(parent, state)));
	}
}