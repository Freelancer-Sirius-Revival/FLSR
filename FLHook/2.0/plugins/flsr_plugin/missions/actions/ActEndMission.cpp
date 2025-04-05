#include "ActEndMission.h"

namespace Missions
{
	void ActEndMission::Execute(Mission& mission, const MissionObject& activator) const
	{
		mission.End();
	}
}