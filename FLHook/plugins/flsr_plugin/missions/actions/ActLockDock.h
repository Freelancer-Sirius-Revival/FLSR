#pragma once
#include "Action.h"

namespace Missions
{
	struct ActLockDock : Action
	{
		uint label = 0;
		uint solarId = 0;
		bool lock = false;

		void Execute(Mission& mission, const MissionObject& activator) const;
	};
}