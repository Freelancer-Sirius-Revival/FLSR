#pragma once
#include "Action.h"
#include "../../Empathies.h"

namespace Missions
{
	struct ActSetVibe : Action
	{
		uint objNameOrLabel = 0;
		uint targetObjNameOrLabel = 0;
		float reputation = 0.0f;

		void Execute(Mission& mission, const MissionObject& activator) const;
	};
	typedef std::shared_ptr<ActSetVibe> ActSetVibePtr;
}