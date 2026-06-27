#include "ActActTrig.h"
#include <random>

namespace Missions
{
	static std::random_device rd;
	static std::mt19937 gen(rd());

	void ActActTrig::Execute(Mission& mission, const MissionObject& activator) const
	{
		uint targetTriggerId = 0;
		if (triggers.size() == 1)
		{
			targetTriggerId = triggers.at(0).triggerId;
			const float probability = triggers.at(0).probability;
			if (probability <= 0.0f || (probability < 1.0f && std::uniform_real_distribution<float>(0, 1)(gen) > probability))
				return;
		}
		else if (triggers.size() > 1)
		{
			std::vector<float> weights;
			for (const auto& entry : triggers)
				weights.push_back(entry.probability);
			const auto index = std::discrete_distribution<size_t>(weights.begin(), weights.end())(gen);
			targetTriggerId = triggers.at(index).triggerId;
		}
		else
		{
			ConPrint(L"ERROR: Msn " + stows(mission.name) + L": Act_ActTrig has no target trigger!\n");
			return;
		}

		if (const auto& triggerEntry = mission.triggers.find(targetTriggerId); triggerEntry != mission.triggers.end())
		{
			if (activate)
			{
				if (branching)
				{
					if (activator.id != 0)
						triggerEntry->second.CreateBranch(activator);
					else
						ConPrint(L"ERROR: Msn " + stows(mission.name) + L": Act_ActTrig has no set Activator for branching " + std::to_wstring(targetTriggerId) + L"!\n");
				}
				else
					triggerEntry->second.Activate();
			}
			else
				triggerEntry->second.Deactivate();
		}
		else
			ConPrint(L"ERROR: Msn " + stows(mission.name) + L": Act_ActTrig could not find trigger " + std::to_wstring(targetTriggerId) + L"!\n");
	}
}
