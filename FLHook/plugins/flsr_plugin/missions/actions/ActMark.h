#pragma once
#include "Action.h"

namespace Missions
{
	struct ActMark : Action
	{
		uint label = 0;
		uint targetObjNameOrLabel = 0;
		bool marked = false;

		void Execute(Mission& mission, const MissionObject& activator) const;
	};
	typedef std::shared_ptr<ActMark> ActMarkPtr;
}