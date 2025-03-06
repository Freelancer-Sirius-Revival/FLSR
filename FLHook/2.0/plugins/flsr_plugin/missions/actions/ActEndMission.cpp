#include <FLHook.h>
#include "ActEndMission.h"

namespace Missions
{
	ActEndMission::ActEndMission(const ActionParent& parent) :
		Action(parent, TriggerAction::Act_EndMission)
	{}

	void ActEndMission::Execute()
	{
		ConPrint(stows(missions[parent.missionId].archetype->name) + L"->" + stows(triggers[parent.triggerId].archetype->name) + L": Act_EndMission\n");
		missions[parent.missionId].End();
	}
}