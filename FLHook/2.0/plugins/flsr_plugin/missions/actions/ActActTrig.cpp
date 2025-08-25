#include "ActActTrig.h"
#include <random>

namespace Missions
{
	static std::random_device rd;
	static std::mt19937 gen(rd());

	void ActActTrig::Execute(Mission& mission, const MissionObject& activator) const
	{
		if (probability <= 0.0f || (probability < 1.0f && std::uniform_real_distribution<float>(0, 1)(gen) > probability) )
			return;
		for (auto& trigger : mission.triggers)
		{
			if (trigger.nameId == triggerId)
			{
				activate ? trigger.Activate() : trigger.Deactivate();
				return;
			}
		}
		ConPrint(L"ERROR: Act_ActTrig could not find trigger " + std::to_wstring(triggerId) + L"\n");
	}
}
