#pragma once
#include "Action.h"

namespace Missions
{
	struct ActActTrigEntry
	{
		uint triggerId = 0;
		float probability = 1.0f;
	};

	struct ActActTrig : Action
	{
		std::vector<ActActTrigEntry> triggers;
		bool activate = false;

		void Execute(Mission& mission, const MissionObject& activator) const;
	};
	typedef std::shared_ptr<ActActTrig> ActActTrigPtr;
}