#pragma once
#include "Action.h"

namespace Missions
{
	struct ActTerminateMsn : Action
	{
		bool deleteMission = false;
		void Execute(Mission& mission, const MissionObject& activator) const;
	};
	typedef std::shared_ptr<ActTerminateMsn> ActTerminateMsnPtr;
}