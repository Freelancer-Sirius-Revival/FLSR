#pragma once
#include "Action.h"
#include "../Mission.h"

namespace Missions
{
	struct ActSetMsnResult : Action
	{
		Mission::MissionResult result = Mission::MissionResult::Failure;
		uint failureStringId = 0;

		void Execute(Mission& mission, const MissionObject& activator) const;
	};
	typedef std::shared_ptr<ActSetMsnResult> ActSetMsnResultPtr;
}