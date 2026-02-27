#pragma once
#include "Action.h"
#include "../Mission.h"

namespace Missions
{
	enum class LeaveMsnType
	{
		Silent,
		Success,
		Failure
	};

	struct ActLeaveMsn : Action
	{
		uint label = 0;
		LeaveMsnType leaveType = LeaveMsnType::Silent;
		uint failureStringId = 0;

		void Execute(Mission& mission, const MissionObject& activator) const;
	};
	typedef std::shared_ptr<ActLeaveMsn> ActLeaveMsnPtr;
}