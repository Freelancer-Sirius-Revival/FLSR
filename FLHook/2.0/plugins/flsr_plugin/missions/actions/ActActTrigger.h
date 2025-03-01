#pragma once
#include "Action.h"
#include "ActActTriggerArch.h"

namespace Missions
{
	struct ActActTrigger : Action
	{
		std::string triggerName;
		bool activate;

		ActActTrigger(Trigger* parentTrigger, const ActActTriggerArchetype* archetype);
		void Execute();
	};
}