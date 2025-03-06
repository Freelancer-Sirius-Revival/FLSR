#pragma once
#include "Action.h"
#include "ActDestroyArch.h"

namespace Missions
{
	struct ActDestroy : Action
	{
		const ActDestroyArchetypePtr archetype;

		ActDestroy(const ActionParent& parent, const ActDestroyArchetypePtr actionArchetype);
		void Execute();
	};
}