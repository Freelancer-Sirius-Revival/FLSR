#pragma once
#include <FLHook.h>

namespace NpcShipArchetypes
{
	struct NpcShipArch
	{
		uint id = 0;
		uint archetypeId = 0;
		uint loadoutId = 0;
		std::string stateGraph = "";
		uint pilotId = 0;
		byte level = 0;
	};

	bool GetNpcShipArch(const uint id, NpcShipArch& result);
    void ReadFiles();
}