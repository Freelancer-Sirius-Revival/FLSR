#pragma once
#include "Action.h"
#include "ActSpawnSolarArch.h"

namespace Missions
{
	struct ActSpawnSolar : Action
	{
		const std::string solarName;

		ActSpawnSolar(Trigger* parentTrigger, const ActSpawnSolarArchetype* archetype);
		void Execute();
	};
}