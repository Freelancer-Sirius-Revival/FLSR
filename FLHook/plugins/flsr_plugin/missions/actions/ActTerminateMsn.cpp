#include "ActTerminateMsn.h"

namespace Missions
{
	void ActTerminateMsn::Execute(Mission& mission, const MissionObject& activator) const
	{
		mission.End();
	}
}