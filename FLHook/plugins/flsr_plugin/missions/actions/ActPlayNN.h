#pragma once
#include "Action.h"

namespace Missions
{
	struct ActPlayNN : Action
	{
		uint label = 0;
		std::vector<uint >soundIds;

		void Execute(Mission& mission, const MissionObject& activator) const;
	};
	typedef std::shared_ptr<ActPlayNN> ActPlayNNPtr;
}