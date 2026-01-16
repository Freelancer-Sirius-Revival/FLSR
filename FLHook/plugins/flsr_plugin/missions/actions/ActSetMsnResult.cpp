#include "ActSetMsnResult.h"

namespace Missions
{
	void ActSetMsnResult::Execute(Mission& mission, const MissionObject& activator) const
	{
		mission.missionResult = result;
	}
}