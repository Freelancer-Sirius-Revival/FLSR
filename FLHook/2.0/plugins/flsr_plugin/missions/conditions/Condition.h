#pragma once
#include "../Trigger.h"

namespace Missions
{
	struct Activator
	{
		unsigned int objId = 0;
		unsigned int clientId = 0;
	};

	struct Condition
	{
		Trigger* trigger;
		const TriggerCondition type;
		Activator activator;

		Condition(Trigger* parentTrigger, const TriggerCondition cndType) :
			trigger(parentTrigger),
			type(cndType)
		{}
		virtual ~Condition()
		{}
		virtual void Register() {};
		virtual void Unregister() {};
	};
}