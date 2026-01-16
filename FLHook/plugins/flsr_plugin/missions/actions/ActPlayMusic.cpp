#include "ActPlayMusic.h"

namespace Missions
{
	static void PlayMusic(const uint clientId, const pub::Audio::Tryptich& music)
	{
		if (!HkIsValidClientID(clientId) || HkIsInCharSelectMenu(clientId))
			return;
		pub::Audio::SetMusic(clientId, music);
	}

	void ActPlayMusic::Execute(Mission& mission, const MissionObject& activator) const
	{
		if (label == Activator)
		{
			if (activator.type == MissionObjectType::Client && activator.id)
				PlayMusic(activator.id, music);
		}
		else if (const auto& objectsByLabel = mission.objectsByLabel.find(label); objectsByLabel != mission.objectsByLabel.end())
		{
			for (const auto& object : objectsByLabel->second)
			{
				if (object.type == MissionObjectType::Client)
					PlayMusic(object.id, music);
			}
		}
	}
}