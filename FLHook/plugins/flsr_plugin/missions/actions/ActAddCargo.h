#pragma once
#include "Action.h"

namespace Missions
{
	struct ActAddCargo : Action
	{
		uint label = 0;
		uint itemId = 0;
		uint count = 1;
		bool missionFlagged = false;

		void Execute(Mission& mission, const MissionObject& activator) const;
	};
}