#include "CndTimer.h"
#include "../Mission.h"

namespace Missions
{
	std::unordered_set<CndTimer*> timerConditions;

	CndTimer::CndTimer(const ConditionParent& parent, const CndTimerArchetypePtr conditionArchetype) :
		Condition(parent, ConditionType::Cnd_Timer),
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
		auto& mission = missions.at(parent.missionId);
		auto& trigger = mission.triggers.at(parent.triggerId);
		passedTimeInS += elapsedTimeInS;
		if (passedTimeInS >= archetype->timeInS)
		{
			ConPrint(stows(mission.archetype->name) + L"->" + stows(trigger.archetype->name) + L": Cnd_Timer " + std::to_wstring(archetype->timeInS) + L"s \n");
			trigger.activator.type = MissionObjectType::Client;
			trigger.activator.id = 0;
			return true;
		}
		return false;
	}
}