#pragma once
#include <FLHook.h>

namespace RandomMissions
{
	struct NpcShip
	{
		uint id = 0;
		uint archetypeId = 0;
		uint loadoutId = 0;
		std::string stateGraph = "";
		uint pilotId = 0;
		byte level = 0;
		std::unordered_set<byte> difficulties;
	};

	extern std::unordered_map<uint, NpcShip> npcShipById;

	void ReadNpcShipData();
}