#include "ActActTrigger.h"
#include <random>

namespace Missions
{
	static std::random_device rd;
	static std::mt19937 gen(rd());

	void ActActTrigger::Execute(Mission& mission, const MissionObject& activator) const
	{
		if (probability < 1.0f && std::uniform_real_distribution<float>(0, 1)(gen) < probability)
			return;
		for (auto& triggerEntry : mission.triggers)
		{
			if (triggerEntry.second.archetype->name == triggerName)
			{
				activate ? triggerEntry.second.Activate() : triggerEntry.second.Deactivate();
				return;
			}
		}
	}
}