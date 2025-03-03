#pragma once
#include "Action.h"
#include "ActSpawnSolarArch.h"

namespace Missions
{
	struct ActSpawnSolar : Action
	{
		const ActSpawnSolarArchetypePtr archetype;

		ActSpawnSolar(Trigger* parentTrigger, const ActSpawnSolarArchetypePtr actionArchetype);
		void Execute();
	};
}