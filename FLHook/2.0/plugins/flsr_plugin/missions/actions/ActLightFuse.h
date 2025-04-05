#pragma once
#include "Action.h"

namespace Missions
{
	struct ActLightFuse : Action
	{
		uint objNameOrLabel = 0;
		std::string fuseName = "";

		void Execute(Mission& mission, const MissionObject& activator) const;
	};
	typedef std::shared_ptr<ActLightFuse> ActLightFusePtr;
}