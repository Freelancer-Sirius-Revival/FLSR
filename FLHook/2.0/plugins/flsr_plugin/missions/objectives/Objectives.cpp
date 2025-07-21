#include <FLHook.h>
#include "Objectives.h"
#include "Objective.h"

namespace Missions
{
	Objectives::Objectives(const uint id, const uint missionId) :
		id(id),
		missionId(missionId)
	{}

	void Objectives::Progress(const uint objId, const int index) const
	{
		if (index >= 0 && index < objectives.size())
			objectives[index]->Execute(objId);
	}
}