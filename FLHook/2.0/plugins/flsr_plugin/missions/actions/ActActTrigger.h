#pragma once
#include "Action.h"

namespace Missions
{
	struct ActActTrigger : Action
	{
		uint nameId = 0;
		bool activate = false;
		float probability = 1.0f;

		void Execute(Mission& mission, const MissionObject& activator) const;
	};
	typedef std::shared_ptr<ActActTrigger> ActActTriggerPtr;
}