#pragma once
#include "Condition.h"
#include "CndTimerArch.h"

namespace Missions
{
	struct CndTimer : Condition
	{
		CndTimerArchetypePtr archetype;
		float targetTimeInS;
		float passedTimeInS;

		CndTimer(const ConditionParent& parent, const CndTimerArchetypePtr conditionArchetype);
		~CndTimer();
		void Register();
		void Unregister();
		bool Matches(const float elapsedTimeInS);
	};

	extern std::unordered_set<CndTimer*> timerConditions;
}