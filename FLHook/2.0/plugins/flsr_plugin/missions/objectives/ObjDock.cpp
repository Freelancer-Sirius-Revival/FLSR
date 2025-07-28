#include "ObjDock.h"
#include "ObjCndTrue.h"
#include "../Mission.h"

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

		auto& mission = missions.at(parent.missionId);
		pub::AI::DirectiveDockOp dockOp;
		dockOp.fireWeapons = false;
		dockOp.dockTargetObjId = 0;
		if (const auto& objectEntry = mission.objectIdsByName.find(targetObjNameOrId); objectEntry != mission.objectIdsByName.end())
			dockOp.dockTargetObjId = objectEntry->second;
		if (!dockOp.dockTargetObjId)
			dockOp.dockTargetObjId = targetObjNameOrId;
		dockOp.dockTargetDirectionObjId = 0;
		dockOp.x12 = 0;
		dockOp.dockPortIndex = -1;
		dockOp.x1C = 0;
		dockOp.x20 = 200.0f;
		dockOp.x24 = 500.0f;
		dockOp.x28 = 0;
		pub::AI::SubmitDirective(objId, &dockOp);

		const auto& condition = ConditionPtr(new ObjCndTrue(parent, objectiveIndex, objId));
		mission.objectiveConditionByObjectId.insert({ objId, condition });
		condition->Register();
	}
}