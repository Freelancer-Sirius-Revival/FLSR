#pragma once
#include "ActActTrig.h"

namespace Missions
{
	struct ActActMsnTrig : public ActActTrig
	{
		uint missionId = 0;

		void Execute(Mission& mission, const MissionObject& activator) const;
	};
	typedef std::shared_ptr<ActActMsnTrig> ActActMsnTrigPtr;
}