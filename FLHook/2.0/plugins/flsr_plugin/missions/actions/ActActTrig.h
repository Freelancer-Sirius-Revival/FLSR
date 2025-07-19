#pragma once
#include "Action.h"

namespace Missions
{
	struct ActActTrig : Action
	{
		uint triggerId = 0;
		bool activate = false;
		float probability = 1.0f;

		void Execute(Mission& mission, const MissionObject& activator) const;
	};
	typedef std::shared_ptr<ActActTrig> ActActTrigPtr;
}