#pragma once
#include <FLHook.h>
#include "Objective.h"

namespace Missions
{
	Objective* TryReadObjectiveFromIni(const ObjectiveParent& objectiveParent, INI_Reader& ini);
}