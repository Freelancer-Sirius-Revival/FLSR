#include "ActTerminateMsn.h"
#include "../MissionBoard.h"
#include "../Missions.h"

namespace Missions
{
	void ActTerminateMsn::Execute(Mission& mission, const MissionObject& activator) const
	{
		mission.End();

		if (mission.offerId)
		{
			MissionBoard::DeleteMissionOffer(mission.offerId);
			mission.offerId = 0;
		}

		if ( mission.offer.reofferCondition == MissionReofferCondition::Always ||
			(mission.offer.reofferCondition == MissionReofferCondition::OnFail && mission.missionResult == Mission::MissionResult::Failure) ||
			(mission.offer.reofferCondition == MissionReofferCondition::OnSuccess && mission.missionResult == Mission::MissionResult::Success))
		{
			RegisterMissionToJobBoard(mission);
		}
	}
}