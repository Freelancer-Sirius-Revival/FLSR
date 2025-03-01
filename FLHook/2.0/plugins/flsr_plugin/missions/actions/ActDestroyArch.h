#pragma once
#include "FLCoreServer.h"

namespace Missions
{
	struct ActDestroyArchetype
	{
		std::string objNameOrLabel = "";
		DestroyType destroyType = DestroyType::VANISH;
	};
}