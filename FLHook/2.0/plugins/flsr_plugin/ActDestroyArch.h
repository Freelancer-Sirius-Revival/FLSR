#pragma once
#include "FLCoreServer.h"

namespace Missions
{
	struct ActDestroyArchetype
	{
		std::string objName = "";
		DestroyType destroyType = DestroyType::VANISH;
	};
}