#pragma once
#include "Action.h"
#include "ActGiveObjListArch.h"

namespace Missions
{
	struct ActGiveObjList : Action
	{
		const ActGiveObjListArchetypePtr archetype;

		ActGiveObjList(const ActionParent& parent, const ActGiveObjListArchetypePtr actionArchetype);
		void Execute();
	};
}