#include <FLHook.h>
#include "ActActTrigger.h"

namespace Missions
{
	ActActTrigger::ActActTrigger(Trigger* parentTrigger, const ActActTriggerArchetype* archetype) :
		Action(parentTrigger, TriggerAction::Act_LightFuse),
		triggerName(archetype->triggerName),
		activate(archetype->activate)
	{}

	void ActActTrigger::Execute()
	{
		ConPrint(stows(trigger->mission->name) + L"->" + stows(trigger->name) + L": " + (activate ? L"Act_ActTrigger" : L"Act_DeactTrigger") + L" " + stows(triggerName));
		for (const auto& trigger : trigger->mission->triggers)
		{
			if (trigger->name == triggerName)
			{
				ConPrint(L"\n");
				activate ? trigger->Activate() : trigger->Deactivate();
				return;
			}
		}
		ConPrint(L" NOT FOUND\n");
	}
}