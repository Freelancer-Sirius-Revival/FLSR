#include <FLHook.h>
#include "ActEndMission.h"

namespace Missions
{
	ActEndMission::ActEndMission(Trigger* parentTrigger) :
		Action(parentTrigger, TriggerAction::Act_EndMission)
	{}

	void ActEndMission::Execute()
	{
		ConPrint(stows(trigger->mission->name) + L"->" + stows(trigger->name) + L": Act_EndMission\n");
		trigger->mission->End();
	}
}