#include "CndTimer.h"
#include <random>
#include "../Mission.h"
#include "../../Plugin.h"

namespace Missions
{
	static std::random_device rd;
	static std::mt19937 gen(rd());

	std::unordered_set<CndTimer*> registeredConditions;

	CndTimer::CndTimer(const ConditionParent& parent, const float lowerTimeInS, const float upperTimeInS) :
		Condition(parent),
		lowerTimeInS(lowerTimeInS),
		upperTimeInS(upperTimeInS),
		targetTimeInS(0.0f),
		passedTimeInS(0.0f)
	{}

	CndTimer::~CndTimer()
	{
		Unregister();
	}

	void CndTimer::Register()
	{
		passedTimeInS = 0;
		if (lowerTimeInS >= upperTimeInS)
			targetTimeInS = lowerTimeInS;
		else
			targetTimeInS = std::uniform_real_distribution<float>(lowerTimeInS, upperTimeInS)(gen);

		registeredConditions.insert(this);
	}

	void CndTimer::Unregister()
	{
		registeredConditions.erase(this);
	}

	bool CndTimer::Matches(const float elapsedTimeInS)
	{
		const auto& mission = missions.at(parent.missionId);
		passedTimeInS += elapsedTimeInS;
		if (passedTimeInS >= targetTimeInS)
		{
			activator.type = MissionObjectType::Client;
			activator.id = 0;
			return true;
		}
		return false;
	}

	namespace Hooks
	{
		namespace CndTimer
		{
			void __stdcall Elapse_Time_AFTER(float seconds)
			{
				returncode = DEFAULT_RETURNCODE;

				const std::unordered_set<Missions::CndTimer*> currentConditions(registeredConditions);
				for (const auto& condition : currentConditions)
				{
					if (registeredConditions.contains(condition) && condition->Matches(seconds))
						condition->ExecuteTrigger();
				}
			}
		}
	}
}