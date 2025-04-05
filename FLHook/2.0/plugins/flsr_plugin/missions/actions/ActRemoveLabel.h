#pragma once
#include "Action.h"

namespace Missions
{
	struct ActRemoveLabel : Action
	{
		unsigned int objNameOrLabel = 0;
		unsigned int label = 0;

		void Execute(Mission& mission, const MissionObject& activator) const;
	};
	typedef std::shared_ptr<ActRemoveLabel> ActRemoveLabelPtr;
}