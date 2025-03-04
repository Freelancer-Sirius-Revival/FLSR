#pragma once
#include "Action.h"
#include "ActSendCommArch.h"

namespace Missions
{
	struct ActSendComm : Action
	{
		const ActSendCommArchetypePtr archetype;

		ActSendComm(Trigger* parentTrigger, const ActSendCommArchetypePtr actionArchetype);
		void Execute();
	};
}