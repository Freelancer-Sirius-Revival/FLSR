#pragma once
#include "Action.h"
#include "ActAddCargoArch.h"

namespace Missions
{
	struct ActAddCargo : Action
	{
		const ActAddCargoArchetypePtr archetype;

		ActAddCargo(const ActionParent& parent, const ActAddCargoArchetypePtr actionArchetype);
		void Execute();
	};
}