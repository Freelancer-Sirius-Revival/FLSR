#pragma once
#include "Action.h"

namespace Missions
{
	struct ActDestroy : Action
	{
		uint objNameOrLabel = 0;
		DestroyType destroyType = DestroyType::VANISH;

		void Execute(Mission& mission, const MissionObject& activator) const;
	};
	typedef std::shared_ptr<ActDestroy> ActDestroyPtr;
}