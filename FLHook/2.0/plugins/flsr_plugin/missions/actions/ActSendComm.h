#pragma once
#include "Action.h"
#include "ActSendCommArch.h"

namespace Missions
{
	struct ActSendComm : Action
	{
		const ActSendCommArchetypePtr archetype;

		ActSendComm(const ActionParent& parent, const ActSendCommArchetypePtr actionArchetype);
		void Execute();
	};
}