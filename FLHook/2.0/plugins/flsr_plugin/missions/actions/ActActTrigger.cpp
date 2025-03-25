#include "ActActTrigger.h"
#include "../Mission.h"
#include <random>

namespace Missions
{
	static std::random_device rd;
	static std::mt19937 gen(rd());

	ActActTrigger::ActActTrigger(const ActionParent& parent, const ActActTriggerArchetypePtr actionArchetype) :
		Action(parent, ActionType::Act_LightFuse),
		archetype(actionArchetype),
		activate(archetype->activate)
	{}

	void ActActTrigger::Execute()
	{
		auto& mission = missions.at(parent.missionId);
		const auto& trigger = mission.triggers.at(parent.triggerId);
		ConPrint(stows(mission.archetype->name) + L"->" + stows(trigger.archetype->name) + L": " + (activate ? L"Act_ActTrigger" : L"Act_DeactTrigger") + L" " + stows(archetype->triggerName));
		if (archetype->probability < 1.0f && std::uniform_real_distribution<float>(0, 1)(gen) < archetype->probability)
		{
			ConPrint(L" randomly did not occur.\n");
			return;
		}
		for (auto& triggerEntry : mission.triggers)
		{
			if (triggerEntry.second.archetype->name == archetype->triggerName)
			{
				ConPrint(L"\n");
				activate ? triggerEntry.second.Activate() : triggerEntry.second.Deactivate();
				return;
			}
		}
		ConPrint(L" NOT FOUND\n");
	}
}