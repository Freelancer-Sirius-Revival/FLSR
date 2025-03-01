#pragma once
#include "Action.h"
#include "ActAddLabelArch.h"

namespace Missions
{
	struct ActAddLabel : Action
	{
		const std::string objNameOrLabel;
		const std::string label;

		ActAddLabel(Trigger* parentTrigger, const ActAddLabelArchetype* archetype);
		void Execute();
	};
}