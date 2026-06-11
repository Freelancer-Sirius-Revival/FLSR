#include "ActPlayNN.h"

namespace Missions
{
	static void PlaySoundEffect(const uint clientId, const std::vector<uint>& soundIds)
	{
		if (!HkIsValidClientID(clientId) || HkIsInCharSelectMenu(clientId))
			return;
		for (const uint soundId : soundIds)
			pub::Player::SendNNMessage(clientId, soundId);
	}

	void ActPlayNN::Execute(Mission& mission, const MissionObject& activator) const
	{
		if (label == Activator)
		{
			if (activator.type == MissionObjectType::Client && activator.id)
				PlaySoundEffect(activator.id, soundIds);
		}
		else if (const auto& objectsByLabel = mission.objectsByLabel.find(label); objectsByLabel != mission.objectsByLabel.end())
		{
			for (const auto& object : objectsByLabel->second)
			{
				if (object.type == MissionObjectType::Client)
					PlaySoundEffect(object.id, soundIds);
			}
		}
	}
}