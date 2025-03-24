#pragma once
#include "Action.h"
#include "ActDebugMsgArch.h"

namespace Missions
{
	struct ActDebugMsg : Action
	{
		const ActDebugMsgArchetypePtr archetype;

		ActDebugMsg(const ActionParent& parent, const ActDebugMsgArchetypePtr actionArchetype);
		void Execute();
	};
}