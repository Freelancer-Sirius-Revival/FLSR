#include <FLHook.h>
#include "ActDebugMsg.h"
#include "../Mission.h"

namespace Missions
{
	ActDebugMsg::ActDebugMsg(const ActionParent& parent, const ActDebugMsgArchetypePtr actionArchetype) :
		Action(parent, ActionType::Act_DebugMsg),
		archetype(actionArchetype)
	{}

	void ActDebugMsg::Execute()
	{
		const auto& mission = missions.at(parent.missionId);
		const auto& trigger = mission.triggers.at(parent.triggerId);
		const std::wstring text = stows(mission.archetype->name) + L"->" + stows(trigger.archetype->name) + L": " + stows(archetype->message);
		ConPrint(text + L"\n");
		for (const auto& clientId : mission.clientIds)
			PrintUserCmdText(clientId, text);
	}
}