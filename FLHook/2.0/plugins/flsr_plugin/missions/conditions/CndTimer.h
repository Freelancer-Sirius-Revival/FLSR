#pragma once
#include "Condition.h"
#include "CndTimerArch.h"

namespace Missions
{
	struct CndTimer : Condition
	{
		CndTimerArchetypePtr archetype;
		float passedTimeInS;

		CndTimer(Trigger* parentTrigger, const CndTimerArchetypePtr conditionArchetype);
		void Register();
		void Unregister();
		bool Matches(const float elapsedTimeInS);
	};

	extern std::unordered_set<CndTimer*> timerConditions;
}