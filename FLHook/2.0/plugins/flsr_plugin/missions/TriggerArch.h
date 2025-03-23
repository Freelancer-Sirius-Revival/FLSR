#pragma once
#include <iostream>
#include <vector>
#include "actions/ActionTypes.h"
#include "conditions/ConditionTypes.h"

namespace Missions
{
	typedef std::pair<ConditionType, std::shared_ptr<void>> TriggerArchConditionEntry;
	typedef std::pair<ActionType, std::shared_ptr<void>> TriggerArchActionEntry;

	struct TriggerArchetype
	{
		std::string name = "";
		bool initiallyActive = false;
		bool repeatable = false;
		TriggerArchConditionEntry condition = { ConditionType::Cnd_True, nullptr };
		std::vector<TriggerArchActionEntry> actions;
	};
	typedef std::shared_ptr<TriggerArchetype> TriggerArchetypePtr;
}