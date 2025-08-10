#include "ObjFollow.h"
#include "ObjCndTrue.h"

namespace Missions
{
	ObjFollow::ObjFollow(const ObjectiveParent& parent, const uint targetObjName, const float maxDistance, const Vector& position) :
		Objective(parent),
		targetObjName(targetObjName),
		maxDistance(maxDistance),
		position(position)
	{}

	void ObjFollow::Execute(const ObjectiveState& state) const
	{
		if (pub::SpaceObj::ExistsAndAlive(state.objId) != 0)
			return;

		Objective::Execute(state);

		const auto& mission = missions.at(parent.missionId);
		const auto& objectEntry = mission.objectIdsByName.find(targetObjName);
		if (objectEntry != mission.objectIdsByName.end())
		{
			pub::AI::DirectiveFollowOp followOp;
			followOp.fireWeapons = !state.enforceObjective;
			followOp.followSpaceObj = objectEntry->second;
			followOp.maxDistance = maxDistance;
			followOp.offset = position;
			followOp.dunno2 = 400.0f;
			pub::AI::SubmitDirective(state.objId, &followOp);
		}

		RegisterCondition(state.objId, ConditionPtr(new ObjCndTrue(parent, state)));
	}
}