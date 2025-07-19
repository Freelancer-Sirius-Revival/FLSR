#pragma once
#include "Action.h"

namespace Missions
{
	struct ActRelocate : Action
	{
		uint objName = 0;
		Vector position;
		Matrix orientation = { { std::numeric_limits<float>::infinity(), 0, 0 } };

		void Execute(Mission& mission, const MissionObject& activator) const;
	};
	typedef std::shared_ptr<ActRelocate> ActRelocatePtr;
}