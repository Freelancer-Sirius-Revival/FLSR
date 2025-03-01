#include "ActActTrigger.h"
#include <FLHook.h>

namespace Missions
{
	ActActTrigger::ActActTrigger(Trigger* parentTrigger, const ActActTriggerArchetype* archetype) :
		Action(parentTrigger, TriggerAction::Act_LightFuse),
		triggerName(archetype->triggerName),
		activate(archetype->activate)
	{}

	void ActActTrigger::Execute()
	{
		const std::wstring outputPretext = stows(trigger->mission->name) + L"->" + stows(trigger->name) + L": " + (activate ? L"Act_ActTrigger" : L"Act_DeactTrigger") + L" " + stows(triggerName);
		for (const auto& trigger : trigger->mission->triggers)
		{
			if (trigger->name == triggerName)
			{
				ConPrint(outputPretext + L"\n");
				activate ? trigger->Activate() : trigger->Deactivate();
				return;
			}
		}
		ConPrint(outputPretext + L" NOT FOUND\n");
	}
}