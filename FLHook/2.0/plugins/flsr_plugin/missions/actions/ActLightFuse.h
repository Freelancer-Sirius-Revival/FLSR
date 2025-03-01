#pragma once
#include "Action.h"
#include "ActLightFuseArch.h"

namespace Missions
{
	struct ActLightFuse : Action
	{
		const std::string objName;
		const unsigned int fuseId;

		ActLightFuse(Trigger* parentTrigger, const ActLightFuseArchetype* archetype);
		void Execute();
	};
}