#include "CndTimer.h"
#include <random>
#include "../Mission.h"
#include "../../Plugin.h"

namespace Missions
{
	static std::random_device rd;
	static std::mt19937 gen(rd());

	std::unordered_set<CndTimer*> observedCndTimer;
	std::vector<CndTimer*> orderedCndTimer;

	CndTimer::CndTimer(const ConditionParent& parent, const float lowerTimeInS, const float upperTimeInS, const uint activatorLabel) :
		Condition(parent),
		lowerTimeInS(lowerTimeInS),
		upperTimeInS(upperTimeInS),
		targetTimeInS(0.0f),
		passedTimeInS(0.0f),
		activatorLabel(activatorLabel)
	{}

	CndTimer::~CndTimer()
	{
		Unregister();
	}

	ConditionPtr CndTimer::Copy(const ConditionParent& newParent, const uint overrideObjNameOrLabel) const
	{
		return ConditionPtr(new CndTimer(newParent, lowerTimeInS, upperTimeInS, overrideObjNameOrLabel));
	}

	void CndTimer::Register()
	{
		passedTimeInS = 0;
		if (lowerTimeInS >= upperTimeInS)
			targetTimeInS = lowerTimeInS;
		else
			targetTimeInS = std::uniform_real_distribution<float>(lowerTimeInS, upperTimeInS)(gen);

		if (observedCndTimer.insert(this).second)
			orderedCndTimer.push_back(this);
	}

	void CndTimer::Unregister()
	{
		observedCndTimer.erase(this);
		if (const auto it = std::find(orderedCndTimer.begin(), orderedCndTimer.end(), this); it != orderedCndTimer.end())
			orderedCndTimer.erase(it);
	}

	bool CndTimer::Matches(const float elapsedTimeInS)
	{
		const auto& mission = missions.at(parent.missionId);
		passedTimeInS += elapsedTimeInS;
		if (passedTimeInS >= targetTimeInS)
		{
			activator = MissionObject(MissionObjectType::Client, 0);
			if (activatorLabel != 0)
			{
				const auto& mission = missions.at(parent.missionId);
				const auto& objectsByLabel = mission.objectsByLabel.find(activatorLabel);
				if (objectsByLabel != mission.objectsByLabel.end() && !objectsByLabel->second.empty())
					activator = objectsByLabel->second.at(0);
			}
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
				const auto currentConditions(orderedCndTimer);
				for (const auto& condition : currentConditions)
				{
					if (observedCndTimer.contains(condition) && condition->Matches(seconds))
						condition->ExecuteTrigger();
				}
				returncode = DEFAULT_RETURNCODE;
			}
		}
	}
}