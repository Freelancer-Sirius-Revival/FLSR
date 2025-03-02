#pragma once
#include "Action.h"

namespace Missions
{
	struct ActEndMission : Action
	{
		ActEndMission(Trigger* parentTrigger);
		void Execute();
	};
}