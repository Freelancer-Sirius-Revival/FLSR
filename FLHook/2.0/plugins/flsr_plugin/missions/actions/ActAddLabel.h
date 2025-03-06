#pragma once
#include "Action.h"
#include "ActAddLabelArch.h"

namespace Missions
{
	struct ActAddLabel : Action
	{
		const ActAddLabelArchetypePtr archetype;

		ActAddLabel(const ActionParent& parent, const ActAddLabelArchetypePtr actionArchetype);
		void Execute();
	};
}