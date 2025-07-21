#include "Objective.h"

namespace Missions
{
	Objective::Objective(const ObjectiveParent& parent, const int objectiveIndex) :
		parent(parent), objectiveIndex(objectiveIndex)
	{}

	void Objective::Execute(const uint objId) const
	{
		pub::AI::DirectiveCancelOp cancelOp;
		cancelOp.fireWeapons = false;
		pub::AI::SubmitDirective(objId, &cancelOp);
	}
}