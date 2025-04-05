#pragma once
#include <iostream>
#include <vector>
#include "conditions/ConditionTypes.h"

namespace Missions
{
	typedef std::pair<ConditionType, std::shared_ptr<void>> TriggerArchConditionEntry;

	struct Action;

	struct TriggerArchetype
	{
		std::string name = "";
		bool initiallyActive = false;
		bool repeatable = false;
		TriggerArchConditionEntry condition = { ConditionType::Cnd_True, nullptr };
		std::vector<std::shared_ptr<Action>> actions;
	};
	typedef std::shared_ptr<TriggerArchetype> TriggerArchetypePtr;
}