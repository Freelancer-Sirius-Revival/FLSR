#pragma once
#include "Action.h"
#include "ActAddLabelArch.h"

namespace Missions
{
	struct ActAddLabel : Action
	{
		const ActAddLabelArchetypePtr archetype;

		ActAddLabel(Trigger* parentTrigger, const ActAddLabelArchetypePtr actionArchetype);
		void Execute();
	};
}