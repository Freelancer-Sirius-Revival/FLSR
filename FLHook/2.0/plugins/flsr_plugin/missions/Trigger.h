#pragma once
#include "TriggerArch.h"
#include "MissionObject.h"
#include "conditions/Condition.h"

namespace Missions
{
	enum class TriggerState
	{
		Inactive,
		Active,
		Finished
	};

	struct Trigger
	{
		const unsigned int id;
		const unsigned int missionId;
		const TriggerArchetypePtr archetype;
		ConditionPtr condition;
		TriggerState state;
		MissionObject activator;

		Trigger(const unsigned int id, const unsigned int missionId, const TriggerArchetypePtr triggerArchetype);
		virtual ~Trigger();
		void Activate();
		void Deactivate();
		void Execute();
	};
}