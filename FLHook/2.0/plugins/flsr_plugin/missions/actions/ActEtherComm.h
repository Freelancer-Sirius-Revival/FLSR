#pragma once
#include "Action.h"
#include "ActEtherCommArch.h"

namespace Missions
{
	struct ActEtherComm : Action
	{
		const ActEtherCommArchetypePtr archetype;

		ActEtherComm(Trigger* parentTrigger, const ActEtherCommArchetypePtr actionArchetype);
		void Execute();
	};
}