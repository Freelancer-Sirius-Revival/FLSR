#pragma once
#include <FLHook.h>
#include "Action.h"

namespace Missions
{
	Action* TryReadActionFromIni(INI_Reader& ini);
}