#pragma once
#include "Action.h"

namespace Missions
{
	struct ActSetNNObj : Action
	{
		uint label = 0;
		uint message = 0;
		uint systemId = 0;
		Vector position;
		bool bestRoute = false;
		uint targetObjName = 0;

		void Execute(Mission& mission, const MissionObject& activator) const;
	};
	typedef std::shared_ptr<ActSetNNObj> ActSetNNObjPtr;
}