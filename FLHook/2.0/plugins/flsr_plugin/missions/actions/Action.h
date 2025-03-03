#pragma once
#include "../Trigger.h"

namespace Missions
{
	const uint Activator = CreateID("activator");

	struct Action
	{
		Trigger* trigger;
		const TriggerAction type;

		Action(Trigger* parentTrigger, const TriggerAction actType) :
			trigger(parentTrigger),
			type(actType)
		{}
		virtual ~Action() {}
		virtual void Execute() = 0;
	};
}