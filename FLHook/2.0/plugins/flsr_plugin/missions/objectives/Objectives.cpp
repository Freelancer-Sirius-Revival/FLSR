#include <FLHook.h>
#include "Objectives.h"
#include "Objective.h"

namespace Missions
{
	Objectives::Objectives(const uint id, const uint missionId) :
		id(id),
		missionId(missionId)
	{}

	void Objectives::Progress(const ObjectiveState& state) const
	{
		if (state.objectiveIndex >= 0 && state.objectiveIndex < objectives.size())
			objectives[state.objectiveIndex]->Execute(state);
	}
}