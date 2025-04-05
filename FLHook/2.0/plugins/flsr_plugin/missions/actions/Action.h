#pragma once
#include "../Mission.h"
#include <FLHook.h>

namespace Missions
{
	const uint Activator = CreateID("activator");

	struct Action
	{
		Action() {}
		virtual ~Action() {}
		virtual void Execute(Mission& mission, const MissionObject& activator) const = 0;
	};
}