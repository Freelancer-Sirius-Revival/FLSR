#include <FLHook.h>
#include "ActActTrigger.h"

namespace Missions
{
	ActActTrigger::ActActTrigger(const ActionParent& parent, const ActActTriggerArchetypePtr actionArchetype) :
		Action(parent, TriggerAction::Act_LightFuse),
		archetype(actionArchetype),
		activate(archetype->activate)
	{}

	void ActActTrigger::Execute()
	{
		ConPrint(stows(missions[parent.missionId].archetype->name) + L"->" + stows(triggers[parent.triggerId].archetype->name) + L": " + (activate ? L"Act_ActTrigger" : L"Act_DeactTrigger") + L" " + stows(archetype->triggerName));
		for (const auto& triggerId : missions[parent.missionId].triggerIds)
		{
			if (triggers[triggerId].archetype->name == archetype->triggerName)
			{
				ConPrint(L"\n");
				activate ? triggers[triggerId].Activate() : triggers[parent.triggerId].Deactivate();
				return;
			}
		}
		ConPrint(L" NOT FOUND\n");
	}
}