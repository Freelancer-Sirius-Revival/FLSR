#pragma once
#include "Action.h"
#include "ActDestroyArch.h"

namespace Missions
{
	struct ActDestroy : Action
	{
		const ActDestroyArchetypePtr archetype;

		ActDestroy(Trigger* parentTrigger, const ActDestroyArchetypePtr actionArchetype);
		void Execute();
	};
}