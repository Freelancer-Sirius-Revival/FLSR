#pragma once
#include "TriggerArch.h"
#include "Mission.h"

namespace Missions
{
	struct Condition;
	struct Action;

	struct Trigger
	{
		const unsigned int id;
		const unsigned int parentMissionId;
		const TriggerArchetypePtr archetype;
		std::shared_ptr<Condition> condition;
		std::vector<std::shared_ptr<Action>> actions;
		bool active;

		Trigger();
		Trigger(const unsigned int id, const unsigned int parentMissionId, const TriggerArchetypePtr triggerArchetype);
		virtual ~Trigger();
		void Activate();
		void Deactivate();
		void QueueExecution();
	};
	extern std::unordered_map<unsigned int, Trigger> triggers;
}