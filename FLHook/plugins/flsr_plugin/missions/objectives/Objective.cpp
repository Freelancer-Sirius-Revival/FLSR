#include "Objective.h"

namespace Missions
{
	Objective::Objective(const ObjectiveParent& parent) : parent(parent)
	{}

	void Objective::Execute(const ObjectiveState& state) const
	{
		pub::AI::DirectiveCancelOp cancelOp;
		pub::AI::SubmitDirective(state.objId, &cancelOp);
	}

	void Objective::RegisterCondition(const uint objId, const ConditionPtr& condition) const
	{
		const auto& insertResult = missions.at(parent.missionId).objectiveConditionByObjectId.insert({ objId, condition });
		// If there is already a condition for this object, replace it. This will automatically unregister the other condition as by their deconstructor.
		if (!insertResult.second)
			insertResult.first->second = condition;
		condition->Register();
	}
}