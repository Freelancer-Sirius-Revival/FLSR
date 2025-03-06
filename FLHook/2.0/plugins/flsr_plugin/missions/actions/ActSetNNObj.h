#pragma once
#include "Action.h"
#include "ActSetNNObjArch.h"

namespace Missions
{
	struct ActSetNNObj : Action
	{
		const ActSetNNObjArchetypePtr archetype;

		ActSetNNObj(const ActionParent& parent, const ActSetNNObjArchetypePtr actionArchetype);
		void Execute();
	};
}