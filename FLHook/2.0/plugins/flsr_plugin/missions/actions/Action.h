#pragma once
#include "../Trigger.h"

namespace Missions
{
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