#pragma once
#include "Action.h"

namespace Missions
{
	struct ActEndMission : Action
	{
		ActEndMission(const ActionParent& parent);
		void Execute();
	};
}