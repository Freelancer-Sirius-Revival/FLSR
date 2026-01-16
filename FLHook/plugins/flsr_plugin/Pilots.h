#pragma once
#include <FLHook.h>

namespace Pilots
{
	pub::AI::Personality GetPilot(const uint pilotId);
	pub::AI::Personality GetPilotWithJob(const uint pilotId, const uint jobId);
	void ReadFiles();
}