#include "ActLeaveMsn.h"

namespace Missions
{
	const uint victoryMusicId = CreateID("music_victory");
	const uint failureMusicId = CreateID("music_failure");
	const FmtStr successText(1231, 0);

	static void LeaveMission(const uint clientId, const ActLeaveMsn& action, Mission& mission)
	{
		mission.RemoveClient(clientId);

		pub::Audio::Tryptich music;
		music.playOnce = true;
		if (action.leaveType == LeaveMsnType::Success)
		{
			music.crossFadeDurationInS = 0.25f;
			music.overrideMusic = victoryMusicId;
			if (HkIsValidClientID(clientId) && !HkIsInCharSelectMenu(clientId))
			{
				pub::Player::DisplayMissionMessage(clientId, successText, MissionMessageType::MissionMessageType_Success, true);
				pub::Audio::SetMusic(clientId, music);
			}
		}
		else if (action.leaveType == LeaveMsnType::Failure)
		{
			const FmtStr failText(action.failureStringId, 0);
			music.crossFadeDurationInS = 1.0f;
			music.overrideMusic = failureMusicId;
			if (HkIsValidClientID(clientId) && !HkIsInCharSelectMenu(clientId))
			{
				pub::Player::DisplayMissionMessage(clientId, failText, MissionMessageType::MissionMessageType_Type1, true);
				pub::Audio::SetMusic(clientId, music);
			}
		}
	}

	void ActLeaveMsn::Execute(Mission& mission, const MissionObject& activator) const
	{
		if (label == Activator)
		{
			if (activator.type == MissionObjectType::Client)
			{
				LeaveMission(activator.id, *this, mission);
			}
		}
		else if (const auto& objectsByLabel = mission.objectsByLabel.find(label); objectsByLabel != mission.objectsByLabel.end())
		{
			for (const auto& object : objectsByLabel->second)
			{
				if (object.type == MissionObjectType::Client)
				{
					LeaveMission(object.id, *this, mission);
				}
			}
		}
	}
}