#pragma once
#include "Action.h"

namespace Missions
{
	struct ActSetShipAndLoadout : Action
	{
		uint label = 0;
		uint shipArchetypeId = 0;
		uint loadoutId = 0;

		void Execute(Mission& mission, const MissionObject& activator) const;
	};
	typedef std::shared_ptr<ActSetShipAndLoadout> ActSetShipAndLoadoutPtr;
}