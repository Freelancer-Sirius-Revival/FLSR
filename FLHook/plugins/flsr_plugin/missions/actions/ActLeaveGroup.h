#pragma once
#include "Action.h"
#include "../Mission.h"

namespace Missions
{
	struct ActLeaveGroup : Action
	{
		uint label = 0;

		void Execute(Mission& mission, const MissionObject& activator) const;
	};
	typedef std::shared_ptr<ActLeaveGroup> ActLeaveGroupPtr;
}