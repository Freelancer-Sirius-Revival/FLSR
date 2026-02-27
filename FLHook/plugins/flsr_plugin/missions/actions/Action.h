#pragma once
#include <memory>
#include <FLHook.h>
#include "../Mission.h"

namespace Missions
{
	const uint Activator = CreateID("activator");

	struct Action
	{
		Action() {}
		virtual ~Action() {}
		virtual void Execute(Mission& mission, const MissionObject& activator) const = 0;
	};
	typedef std::shared_ptr<Action> ActionPtr;
}