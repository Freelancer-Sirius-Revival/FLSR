#pragma once
#include "Action.h"

namespace Missions
{
	struct ActEndMission : Action
	{
		void Execute(Mission& mission, const MissionObject& activator) const;
	};
	typedef std::shared_ptr<ActEndMission> ActEndMissionPtr;
}