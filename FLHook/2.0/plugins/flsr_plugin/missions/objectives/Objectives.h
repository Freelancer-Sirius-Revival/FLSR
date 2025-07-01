#pragma once
#include <memory>
#include <queue>
#include "ObjectivesArch.h"
#include "../Conditions/Condition.h"

namespace Missions
{
	struct Objectives
	{
		const unsigned int parentMissionId;
		std::queue<ObjectiveEntry> objectives;
		const unsigned int objId;
		ConditionPtr currentCondition;

		Objectives(const unsigned int parentMissionId, const unsigned int objId, const std::vector<ObjectiveEntry>& objectives);
		virtual ~Objectives();
		void Progress();
		void Cancel();
	};
}