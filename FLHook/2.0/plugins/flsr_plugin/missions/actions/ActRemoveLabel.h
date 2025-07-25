#pragma once
#include "Action.h"

namespace Missions
{
	struct ActRemoveLabel : Action
	{
		uint objNameOrLabel = 0;
		uint label = 0;

		void Execute(Mission& mission, const MissionObject& activator) const;
	};
	typedef std::shared_ptr<ActRemoveLabel> ActRemoveLabelPtr;
}