#pragma once
#include "Trigger.h"

namespace Missions
{
	struct Condition
	{
		Trigger* trigger;
		const TriggerCondition type;

		Condition(Trigger* parentTrigger, const TriggerCondition cndType) :
			trigger(parentTrigger),
			type(cndType)
		{}
		virtual ~Condition()
		{}
		virtual void Register() = 0;
		virtual void Unregister() {};
	};
}