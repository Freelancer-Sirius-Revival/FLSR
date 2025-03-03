#pragma once
#include "Action.h"
#include "ActLightFuseArch.h"

namespace Missions
{
	struct ActLightFuse : Action
	{
		const ActLightFuseArchetypePtr archetype;

		ActLightFuse(Trigger* parentTrigger, const ActLightFuseArchetypePtr actionArchetype);
		void Execute();
	};
}