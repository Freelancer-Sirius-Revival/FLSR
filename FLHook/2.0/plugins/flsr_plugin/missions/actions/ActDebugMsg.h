#pragma once
#include "Action.h"

namespace Missions
{
	struct ActDebugMsg : Action
	{
		std::string message = "";

		void Execute(Mission& mission, const MissionObject& activator) const;
	};
	typedef std::shared_ptr<ActDebugMsg> ActDebugMsgPtr;
}