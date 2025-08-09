#pragma once
#include <FLHook.h>
#include "Objective.h"

namespace Missions
{
	Objective* TryReadObjectiveFromIni(const ObjectiveParent& objectiveParent, const int objectiveIndex, INI_Reader& ini);
}