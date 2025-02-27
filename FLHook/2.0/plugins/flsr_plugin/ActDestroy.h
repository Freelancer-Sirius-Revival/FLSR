#pragma once
#include "Action.h"
#include "ActDestroyArch.h"
#include "FLCoreServer.h"

namespace Missions
{
	struct ActDestroy : Action
	{
		std::string objName;
		DestroyType destroyType;

		ActDestroy(Trigger* parentTrigger, const ActDestroyArchetype* archetype);
		void Execute();
	};
}