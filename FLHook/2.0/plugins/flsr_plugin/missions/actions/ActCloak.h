#pragma once
#include "Action.h"

namespace Missions
{
	struct ActCloak : Action
	{
		uint objNameOrLabel = 0;
		bool cloaked = false;

		void Execute(Mission& mission, const MissionObject& activator) const;
	};
	typedef std::shared_ptr<ActCloak> ActCloakPtr;
}