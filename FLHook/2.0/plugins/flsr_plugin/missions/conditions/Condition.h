#pragma once
#include "../Trigger.h"
#include "../Trigger.h"
#include "../MissionObject.h"

namespace Missions
{
	const uint Stranger = CreateID("stranger");

	struct Condition
	{
		Trigger* trigger;
		const TriggerCondition type;
		MissionObject activator;

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