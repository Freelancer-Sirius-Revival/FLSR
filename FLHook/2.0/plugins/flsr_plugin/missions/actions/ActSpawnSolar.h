#pragma once
#include "Action.h"

namespace Missions
{
	struct ActSpawnSolar : Action
	{
		std::string solarName = "";

		void Execute(Mission& mission, const MissionObject& activator) const;
	};
	typedef std::shared_ptr<ActSpawnSolar> ActSpawnSolarPtr;
}