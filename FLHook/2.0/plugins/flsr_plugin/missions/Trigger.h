#pragma once
#include "TriggerArch.h"
#include "Mission.h"

namespace Missions
{
	struct Condition;
	struct Action;

	struct Trigger
	{
		std::string name;
		Mission* mission;
		bool repeatable;
		Condition* condition;
		std::vector<Action*> actions;
		bool active;

		Trigger(Mission* parentMission, const TriggerArchetype& triggerArchetype);
		virtual ~Trigger();
		void Activate();
		void Deactivate();
		void QueueExecution();
	};
}