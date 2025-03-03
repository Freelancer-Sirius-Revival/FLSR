#pragma once
#include "Action.h"
#include "ActChangeStateArch.h"

namespace Missions
{
	struct ActChangeState : Action
	{
		const ActChangeStateArchetypePtr archetype;

		ActChangeState(Trigger* parentTrigger, const ActChangeStateArchetypePtr actionArchetype);
		void Execute();
	};
}