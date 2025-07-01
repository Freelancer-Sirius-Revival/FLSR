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
		for (auto& trigger : mission.triggers)
		{
			if (trigger.nameId == nameId)
			{
				activate ? trigger.Activate() : trigger.Deactivate();
				return;
			}
		}
		ConPrint(L"Error: Act_ActTrig could not find trigger " + std::to_wstring(nameId) + L"\n");
	}
}