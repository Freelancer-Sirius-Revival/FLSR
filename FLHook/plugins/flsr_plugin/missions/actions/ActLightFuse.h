#pragma once
#include "Action.h"

namespace Missions
{
	struct ActLightFuse : Action
	{
		uint objNameOrLabel = 0;
		uint fuse = 0;
		float timeOffset = 0.0f;
		float lifetimeOverride = -1.0f;

		void Execute(Mission& mission, const MissionObject& activator) const;
	};
	typedef std::shared_ptr<ActLightFuse> ActLightFusePtr;
}