#pragma once
#include "Action.h"
#include "ActActTriggerArch.h"

namespace Missions
{
	struct ActActTrigger : Action
	{
		const ActActTriggerArchetypePtr archetype;
		bool activate;

		ActActTrigger(Trigger* parentTrigger, const ActActTriggerArchetypePtr actionArchetype);
		void Execute();
	};
}