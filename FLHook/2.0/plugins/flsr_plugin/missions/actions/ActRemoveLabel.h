#pragma once
#include "Action.h"
#include "ActRemoveLabelArch.h"

namespace Missions
{
	struct ActRemoveLabel : Action
	{
		const ActRemoveLabelArchetypePtr archetype;

		ActRemoveLabel(Trigger* parentTrigger, const ActRemoveLabelArchetypePtr actionArchetype);
		void Execute();
	};
}