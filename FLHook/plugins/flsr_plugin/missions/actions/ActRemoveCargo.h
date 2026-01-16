#pragma once
#include "Action.h"

namespace Missions
{
	struct ActRemoveCargo : Action
	{
		uint label = 0;
		uint itemId = 0;
		uint count = 0;

		void Execute(Mission& mission, const MissionObject& activator) const;
	};
	typedef std::shared_ptr<ActRemoveCargo> ActRemoveCargoPtr;
}