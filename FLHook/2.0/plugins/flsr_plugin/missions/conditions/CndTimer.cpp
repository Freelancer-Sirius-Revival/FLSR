#include "CndTimer.h"

namespace Missions
{
	std::unordered_set<CndTimer*> timerConditions;

	CndTimer::CndTimer(Trigger* parentTrigger, const CndTimerArchetypePtr conditionArchetype) :
		Condition(parentTrigger, TriggerCondition::Cnd_Timer),
		archetype(conditionArchetype),
		passedTimeInS(0.0f)
	{}

	void CndTimer::Register()
	{
		timerConditions.insert(this);
	}

	void CndTimer::Unregister()
	{
		timerConditions.erase(this);
	}

	bool CndTimer::Matches(const float elapsedTimeInS)
	{
		passedTimeInS += elapsedTimeInS;
		if (passedTimeInS >= archetype->timeInS)
		{
			ConPrint(stows(trigger->mission->archetype->name) + L"->" + stows(trigger->archetype->name) + L": Cnd_Timer " + std::to_wstring(archetype->timeInS) + L"s \n");
			activator.type = MissionObjectType::Client;
			activator.id = 0;
			return true;
		}
		return false;
	}
}