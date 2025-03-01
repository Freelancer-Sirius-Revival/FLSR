#pragma once
#include "Action.h"
#include "ActChangeStateArch.h"

namespace Missions
{
	struct ActChangeState : Action
	{
		MissionState state;
		const unsigned int failTextId;

		ActChangeState(Trigger* parentTrigger, const ActChangeStateArchetype* archetype);
		void Execute();
	};
}