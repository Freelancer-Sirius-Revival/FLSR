#include "ObjDock.h"
#include "ObjCndTrue.h"

namespace Missions
{
	ObjDock::ObjDock(const ObjectiveParent& parent, const int objectiveIndex, const uint targetObjNameOrId) :
		Objective(parent, objectiveIndex),
		targetObjNameOrId(targetObjNameOrId)
	{}

	void ObjDock::Execute(const uint objId) const
	{
		if (pub::SpaceObj::ExistsAndAlive(objId) != 0)
			return;

		Objective::Execute(objId);

		uint dockTargetObjId = 0;
		const auto& mission = missions.at(parent.missionId);
		if (const auto& objectEntry = mission.objectIdsByName.find(targetObjNameOrId); objectEntry != mission.objectIdsByName.end())
			dockTargetObjId = objectEntry->second;
		if (!dockTargetObjId)
			dockTargetObjId = targetObjNameOrId;

		if (pub::SpaceObj::ExistsAndAlive(dockTargetObjId) == 0)
		{
			pub::AI::DirectiveDockOp dockOp;
			dockOp.fireWeapons = false;
			dockOp.dockTargetObjId = dockTargetObjId;
			dockOp.dockTargetDirectionObjId = 0;
			dockOp.x12 = 0;
			dockOp.dockPortIndex = -1;
			dockOp.x1C = 0;
			dockOp.x20 = 200.0f;
			dockOp.x24 = 500.0f;
			dockOp.x28 = 0;
			pub::AI::SubmitDirective(objId, &dockOp);
		}

		RegisterCondition(objId, ConditionPtr(new ObjCndTrue(parent, objectiveIndex, objId)));
	}
}