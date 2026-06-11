#pragma once
#include "Action.h"

namespace Missions
{
	struct ActUnlightFuse : Action
	{
		uint objNameOrLabel = 0;
		uint fuse = 0;

		void Execute(Mission& mission, const MissionObject& activator) const;
	};
	typedef std::shared_ptr<ActUnlightFuse> ActUnlightFusePtr;
}