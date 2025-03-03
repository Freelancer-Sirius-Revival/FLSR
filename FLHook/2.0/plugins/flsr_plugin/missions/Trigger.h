#pragma once
#include "TriggerArch.h"
#include "Mission.h"

namespace Missions
{
	struct Condition;
	struct Action;

	struct Trigger
	{
		const TriggerArchetypePtr archetype;
		Mission* mission;
		Condition* condition;
		std::vector<Action*> actions;
		bool active;

		Trigger(Mission* parentMission, const TriggerArchetypePtr triggerArchetype);
		virtual ~Trigger();
		void Activate();
		void Deactivate();
		void QueueExecution();
	};
}