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
			ConPrint(L"ERROR: Act_ActTrig has no target trigger\n");
			return;
		}

		if (const auto& triggerEntry = mission.triggers.find(targetTriggerId); triggerEntry != mission.triggers.end())
			activate ? triggerEntry->second.Activate() : triggerEntry->second.Deactivate();
		else
			ConPrint(L"ERROR: Act_ActTrig could not find trigger " + std::to_wstring(targetTriggerId) + L"\n");
	}
}
