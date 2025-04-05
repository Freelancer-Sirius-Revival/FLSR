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

	CndTimer::~CndTimer()
	{
		Unregister();
	}

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
		passedTimeInS += elapsedTimeInS;
		if (passedTimeInS >= archetype->timeInS)
		{
			activator.type = MissionObjectType::Client;
			activator.id = 0;
			return true;
		}
		return false;
	}
}