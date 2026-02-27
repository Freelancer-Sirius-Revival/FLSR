#include "ObjSetPriority.h"
#include "ObjCndTrue.h"

namespace Missions
{
	ObjSetPriority::ObjSetPriority(const ObjectiveParent& parent, const bool enforceObjectives) :
		Objective(parent),
		enforceObjectives(enforceObjectives)
	{}

	void ObjSetPriority::Execute(const ObjectiveState& state) const
	{
		ObjectiveState newState(state);
		newState.enforceObjective = enforceObjectives;
		RegisterCondition(state.objId, ConditionPtr(new ObjCndTrue(parent, newState)));
	}
}