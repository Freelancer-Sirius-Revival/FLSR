#include <FLHook.h>
#include "ActEndMission.h"
#include "../Mission.h"

namespace Missions
{
	ActEndMission::ActEndMission(const ActionParent& parent) :
		Action(parent, ActionType::Act_EndMission)
	{}

	void ActEndMission::Execute()
	{
		auto& mission = missions.at(parent.missionId);
		auto& trigger = mission.triggers.at(parent.triggerId);
		ConPrint(stows(mission.archetype->name) + L"->" + stows(trigger.archetype->name) + L": Act_EndMission\n");
		mission.End();
	}
}