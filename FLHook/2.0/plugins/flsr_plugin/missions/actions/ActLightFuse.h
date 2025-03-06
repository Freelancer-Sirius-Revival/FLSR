#pragma once
#include "Action.h"
#include "ActLightFuseArch.h"

namespace Missions
{
	struct ActLightFuse : Action
	{
		const ActLightFuseArchetypePtr archetype;

		ActLightFuse(const ActionParent& parent, const ActLightFuseArchetypePtr actionArchetype);
		void Execute();
	};
}