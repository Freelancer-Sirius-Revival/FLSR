#include "ActDebugMsg.h"

namespace Missions
{
	void ActDebugMsg::Execute(Mission& mission, const MissionObject& activator) const
	{
		const std::wstring text = stows(mission.name) + L": " + stows(message);
		ConPrint(text + L"\n");
		struct PlayerData* playerData = 0;
		while (playerData = Players.traverse_active(playerData))
			PrintUserCmdText(playerData->iOnlineID, text);
	}
}