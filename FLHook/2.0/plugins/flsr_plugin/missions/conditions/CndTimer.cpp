#include "CndTimer.h"
#include "../Mission.h"
#include <random>

namespace Missions
{
	static std::random_device rd;
	static std::mt19937 gen(rd());

	std::unordered_set<CndTimer*> timerConditions;

	CndTimer::CndTimer(const ConditionParent& parent, const CndTimerArchetypePtr conditionArchetype) :
		Condition(parent, ConditionType::Cnd_Timer),
		archetype(conditionArchetype),
		targetTimeInS(0.0f),
		passedTimeInS(0.0f)
	{}

	CndTimer::~CndTimer()
	{
		Unregister();
	}

	void CndTimer::Register()
	{
		if (archetype->lowerTimeInS >= archetype->upperTimeInS)
			targetTimeInS = archetype->lowerTimeInS;
		else
			targetTimeInS = std::uniform_real_distribution<float>(archetype->lowerTimeInS, archetype->upperTimeInS)(gen);

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
		if (passedTimeInS >= targetTimeInS)
		{
			activator.type = MissionObjectType::Client;
			activator.id = 0;
			return true;
		}
		return false;
	}
}