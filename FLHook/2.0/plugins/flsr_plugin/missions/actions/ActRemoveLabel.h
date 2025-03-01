#pragma once
#include "Action.h"
#include "ActRemoveLabelArch.h"

namespace Missions
{
	struct ActRemoveLabel : Action
	{
		const std::string objNameOrLabel;
		const std::string label;

		ActRemoveLabel(Trigger* parentTrigger, const ActRemoveLabelArchetype* archetype);
		void Execute();
	};
}