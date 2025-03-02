#pragma once
#include <FLHook.h>
#include "Action.h"
#include "ActDestroyArch.h"

namespace Missions
{
	struct ActDestroy : Action
	{
		const std::string objNameOrLabel;
		const DestroyType destroyType;

		ActDestroy(Trigger* parentTrigger, const ActDestroyArchetype* archetype);
		void Execute();
	};
}