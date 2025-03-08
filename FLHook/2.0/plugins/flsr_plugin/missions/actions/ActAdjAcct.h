#pragma once
#include "Action.h"
#include "ActAdjAcctArch.h"

namespace Missions
{
	struct ActAdjAcct : Action
	{
		const ActAdjAcctArchetypePtr archetype;

		ActAdjAcct(const ActionParent& parent, const ActAdjAcctArchetypePtr actionArchetype);
		void Execute();
	};
}