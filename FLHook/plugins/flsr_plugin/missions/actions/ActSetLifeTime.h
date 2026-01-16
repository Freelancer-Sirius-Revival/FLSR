#pragma once
#include "Action.h"

namespace Missions
{
	struct ActSetLifeTime : Action
	{
		uint objNameOrLabel = 0;
		float lifeTime = 1.0f;

		void Execute(Mission& mission, const MissionObject& activator) const;
	};
	typedef std::shared_ptr<ActSetLifeTime> ActSetLifeTimePtr;
}