#pragma once
#include "Action.h"

namespace Missions
{
	struct ActAdjAcct : Action
	{
		uint label = 0;
		uint cash = 0;
		bool splitBetweenPlayers = false;

		void Execute(Mission& mission, const MissionObject& activator) const;
	};
	typedef std::shared_ptr<ActAdjAcct> ActAdjAcctPtr;
}