#pragma once
#include "Action.h"
#include "ActActTriggerArch.h"

namespace Missions
{
	struct ActActTrigger : Action
	{
		const ActActTriggerArchetypePtr archetype;
		bool activate;

		ActActTrigger(const ActionParent& parent, const ActActTriggerArchetypePtr actionArchetype);
		void Execute();
	};
}