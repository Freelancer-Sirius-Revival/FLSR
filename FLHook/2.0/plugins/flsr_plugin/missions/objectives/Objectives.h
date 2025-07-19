#pragma once
#include <memory>
#include <queue>
#include "ObjectivesArch.h"
#include "../Conditions/Condition.h"

namespace Missions
{
	struct Objectives
	{
		const uint missionId;
		std::queue<ObjectiveEntry> objectives;
		const uint objId;
		ConditionPtr currentCondition;

		Objectives(const uint missionId, const uint objId, const std::vector<ObjectiveEntry>& objectives);
		virtual ~Objectives();
		void Progress();
		void Cancel();
	};
}