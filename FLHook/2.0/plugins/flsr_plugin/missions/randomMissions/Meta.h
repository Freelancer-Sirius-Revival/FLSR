#pragma once
#include <FLHook.h>

namespace RandomMissions
{
	extern std::vector<float> npcWaveSizeLikelihoods;
	extern float npcSpawnDistance;
	extern float solarSpawnDistance;
	extern float secondWaypointDistance;
	extern float npcStayInRangeDistance;
	extern float playerStayInRangeDistance;
	extern float playerStayInRangeDistanceTolerance;
	extern float returnToMissionTimeSeconds;

	void ReadMetadata();
}