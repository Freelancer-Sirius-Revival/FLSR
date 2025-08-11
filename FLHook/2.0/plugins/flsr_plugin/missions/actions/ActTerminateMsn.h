#pragma once
#include "Action.h"

namespace Missions
{
	struct ActTerminateMsn : Action
	{
		void Execute(Mission& mission, const MissionObject& activator) const;
	};
	typedef std::shared_ptr<ActTerminateMsn> ActTerminateMsnPtr;
}