#pragma once
#include "Action.h"

namespace Missions
{
	struct ActSetDockState : Action
	{
		uint objNameOrLabel = 0;
		std::string dockHardpoint = "";
		bool opened = false;

		void Execute(Mission& mission, const MissionObject& activator) const;
	};
	typedef std::shared_ptr<ActSetDockState> ActSetDockStatePtr;
}