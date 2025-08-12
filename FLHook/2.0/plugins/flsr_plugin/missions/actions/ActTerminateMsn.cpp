#include "ActTerminateMsn.h"
#include "../MissionBoard.h"
#include "../Missions.h"

namespace Missions
{
	void ActTerminateMsn::Execute(Mission& mission, const MissionObject& activator) const
	{
		mission.End();
	}
}