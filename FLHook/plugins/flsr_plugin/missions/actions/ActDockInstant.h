#pragma once
#include "Action.h"

namespace Missions
{
	struct ActDockInstant : Action
	{
		uint label = 0;
		uint targetObjName = 0;
		std::string dockHardpoint = "";

		void Execute(Mission& mission, const MissionObject& activator) const;
	};
	typedef std::shared_ptr<ActDockInstant> ActDockInstantPtr;
}