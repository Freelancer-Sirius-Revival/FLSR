#pragma once
#include "Action.h"

namespace Missions
{
	struct ActSpawnShip : Action
	{
		uint msnNpcId = 0;
		uint objectivesId = 0;
		Vector position = { std::numeric_limits<float>::infinity(), 0, 0 };
		Matrix orientation = { { std::numeric_limits<float>::infinity(), 0, 0 } };

		void Execute(Mission& mission, const MissionObject& activator) const;
	};
	typedef std::shared_ptr<ActSpawnShip> ActSpawnShipPtr;
}