#pragma once
#include <FLHook.h>
#include "Condition.h"

namespace Missions
{
	Condition* TryReadConditionFromIni(const ConditionParent& conditionParent, INI_Reader& ini);
}