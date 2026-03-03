#pragma once
#include "Action.h"
#include "../Mission.h"

namespace Missions
{
	struct ActDisplayMsg : Action
	{
		uint label = 0;
		uint stringId = 0;

		void Execute(Mission& mission, const MissionObject& activator) const;
	};
	typedef std::shared_ptr<ActDisplayMsg> ActDisplayMsgPtr;
}