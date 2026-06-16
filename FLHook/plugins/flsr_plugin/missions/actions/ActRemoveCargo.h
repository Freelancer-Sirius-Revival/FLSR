#pragma once
#include "Action.h"

namespace Missions
{
	struct ActRemoveCargo : Action
	{
		uint label = 0;
		uint itemId = 0;
		uint count = 1;

		void Execute(Mission& mission, const MissionObject& activator) const;
	};
}