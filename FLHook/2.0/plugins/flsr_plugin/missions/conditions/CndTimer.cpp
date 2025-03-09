#include "CndTimer.h"
#include "../Trigger.h"

namespace Missions
{
	std::unordered_set<CndTimer*> timerConditions;

	CndTimer::CndTimer(const ConditionParent& parent, const CndTimerArchetypePtr conditionArchetype) :
		Condition(parent, TriggerCondition::Cnd_Timer),
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
			ConPrint(stows(missions[parent.missionId].archetype->name) + L"->" + stows(triggers[parent.triggerId].archetype->name) + L": Cnd_Timer " + std::to_wstring(archetype->timeInS) + L"s \n");
			triggers[parent.triggerId].activator.type = MissionObjectType::Client;
			triggers[parent.triggerId].activator.id = 0;
			return true;
		}
		return false;
	}
}