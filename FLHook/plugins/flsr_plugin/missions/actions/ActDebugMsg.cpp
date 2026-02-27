#include "ActDebugMsg.h"

namespace Missions
{
	void ActDebugMsg::Execute(Mission& mission, const MissionObject& activator) const
	{
		const std::wstring text = stows(mission.name) + L": " + stows(message);
		ConPrint(text + L"\n");
		for (const auto& clientId : mission.clientIds)
			PrintUserCmdText(clientId, text);
	}
}