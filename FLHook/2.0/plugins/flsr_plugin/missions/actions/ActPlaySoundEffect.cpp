#include "ActPlaySoundEffect.h"

namespace Missions
{
	static void PlaySoundEffect(const uint clientId, const uint soundId)
	{
		if (!HkIsValidClientID(clientId) || HkIsInCharSelectMenu(clientId))
			return;
		pub::Audio::PlaySoundEffect(clientId, soundId);
	}

	void ActPlaySoundEffect::Execute(Mission& mission, const MissionObject& activator) const
	{
		if (label == Activator)
		{
			if (activator.type == MissionObjectType::Client && activator.id)
				PlaySoundEffect(activator.id, soundId);
		}
		else if (const auto& objectsByLabel = mission.objectsByLabel.find(label); objectsByLabel != mission.objectsByLabel.end())
		{
			for (const auto& object : objectsByLabel->second)
			{
				if (object.type == MissionObjectType::Client)
					PlaySoundEffect(object.id, soundId);
			}
		}
	}
}