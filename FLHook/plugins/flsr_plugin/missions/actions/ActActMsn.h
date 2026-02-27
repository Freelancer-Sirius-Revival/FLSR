#pragma once
#include "Action.h"

namespace Missions
{
	const uint ActActMsnAllPlayerLabels = 0;

	struct ActActMsn : Action
	{
		uint missionId = 0;
		std::unordered_set<uint> playerLabelsToTransfer;

		void Execute(Mission& mission, const MissionObject& activator) const;
	};
	typedef std::shared_ptr<ActActMsn> ActActMsnPtr;
}