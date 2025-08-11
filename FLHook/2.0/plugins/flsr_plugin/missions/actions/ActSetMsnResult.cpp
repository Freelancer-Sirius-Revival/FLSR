#include "ActSetMsnResult.h"

namespace Missions
{
	const uint victoryMusicId = CreateID("music_victory");
	const uint failureMusicId = CreateID("music_failure");
	const FmtStr successText(1231, 0);

	void ActSetMsnResult::Execute(Mission& mission, const MissionObject& activator) const
	{
		mission.missionResult = result;

		pub::Audio::Tryptich music;
		music.playOnce = true;
		if (result == Mission::MissionResult::Success)
		{
			music.overrideMusic = victoryMusicId;
			for (const auto& clientId : mission.clientIds)
			{
				if (HkIsValidClientID(clientId) && !HkIsInCharSelectMenu(clientId))
				{
					pub::Player::DisplayMissionMessage(clientId, successText, MissionMessageType::MissionMessageType_Success, true);
					pub::Audio::SetMusic(clientId, music);
				}
			}
		}
		else
		{
			const FmtStr failText(failureStringId, 0);
			music.crossFadeDurationInS = 1.0f;
			music.overrideMusic = failureMusicId;
			for (const auto& clientId : mission.clientIds)
			{
				if (HkIsValidClientID(clientId) && !HkIsInCharSelectMenu(clientId))
				{
					pub::Player::DisplayMissionMessage(clientId, failText, MissionMessageType::MissionMessageType_Type1, true);
					pub::Audio::SetMusic(clientId, music);
				}
			}
		}

		for (const auto& clientId : mission.clientIds)
		{
			uint msnId;
			pub::Player::GetMsnID(clientId, msnId);
			if (msnId == mission.offerId)
				pub::Player::SetMsnID(clientId, 0, 0, false, 0);
		}
	}
}