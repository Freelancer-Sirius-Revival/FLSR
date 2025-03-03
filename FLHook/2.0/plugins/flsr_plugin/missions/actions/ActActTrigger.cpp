#include <FLHook.h>
#include "ActActTrigger.h"

namespace Missions
{
	ActActTrigger::ActActTrigger(Trigger* parentTrigger, const ActActTriggerArchetypePtr actionArchetype) :
		Action(parentTrigger, TriggerAction::Act_LightFuse),
		archetype(actionArchetype),
		activate(archetype->activate)
	{}

	void ActActTrigger::Execute()
	{
		ConPrint(stows(trigger->mission->archetype->name) + L"->" + stows(trigger->archetype->name) + L": " + (activate ? L"Act_ActTrigger" : L"Act_DeactTrigger") + L" " + stows(archetype->triggerName));
		for (const auto& trigger : trigger->mission->triggers)
		{
			if (trigger->archetype->name == archetype->triggerName)
			{
				ConPrint(L"\n");
				activate ? trigger->Activate() : trigger->Deactivate();
				return;
			}
		}
		ConPrint(L" NOT FOUND\n");
	}
}