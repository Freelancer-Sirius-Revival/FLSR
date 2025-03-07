#pragma once
#include "Action.h"
#include "ActSpawnShipArch.h"

namespace Missions
{
	struct ActSpawnShip : Action
	{
		const ActSpawnShipArchetypePtr archetype;

		ActSpawnShip(const ActionParent& parent, const ActSpawnShipArchetypePtr actionArchetype);
		void Execute();
	};
}