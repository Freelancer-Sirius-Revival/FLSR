#include "ActAdjAcct.h"

namespace Missions
{
	static void AddCash(const uint clientId, const int cash)
	{
		int currentCash = MAXINT32;
		pub::Player::InspectCash(clientId, currentCash);
		const auto newCash = static_cast<long long int>(currentCash) + cash;
		if (newCash > MAXINT32)
			pub::Player::AdjustCash(clientId, MAXINT32 - currentCash);
		else if (newCash < 0)
			pub::Player::AdjustCash(clientId, -currentCash);
		else
			pub::Player::AdjustCash(clientId, cash);
	}

	void ActAdjAcct::Execute(Mission& mission, const MissionObject& activator) const
	{
		if (label == Activator)
		{
			if (activator.type == MissionObjectType::Client)
				AddCash(activator.id, cash);
		}
		else if (const auto& objectsByLabel = mission.objectsByLabel.find(label); objectsByLabel != mission.objectsByLabel.end())
		{
			std::vector<uint> clientIds;
			for (const auto& object : objectsByLabel->second)
			{
				if (object.type == MissionObjectType::Client)
					clientIds.push_back(object.id);
			}
			for (const auto& id : clientIds)
			{
				AddCash(id, splitBetweenPlayers ? static_cast<int>(std::trunc(cash / clientIds.size())) : cash);
			}
		}
	}
}