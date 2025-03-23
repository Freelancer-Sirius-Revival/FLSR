#include "ActActTrigger.h"
#include "../Mission.h"

namespace Missions
{
	ActActTrigger::ActActTrigger(const ActionParent& parent, const ActActTriggerArchetypePtr actionArchetype) :
		Action(parent, ActionType::Act_LightFuse),
		archetype(actionArchetype),
		activate(archetype->activate)
	{}

	void ActActTrigger::Execute()
	{
		auto& mission = missions.at(parent.missionId);
		auto& trigger = mission.triggers.at(parent.triggerId);
		ConPrint(stows(mission.archetype->name) + L"->" + stows(trigger.archetype->name) + L": " + (activate ? L"Act_ActTrigger" : L"Act_DeactTrigger") + L" " + stows(archetype->triggerName));
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