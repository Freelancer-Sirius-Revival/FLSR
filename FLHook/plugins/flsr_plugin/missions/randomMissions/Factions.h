#pragma once
#include <FLHook.h>

namespace RandomMissions
{
	struct Faction
	{
		uint id;
		std::unordered_set<uint> hostileFactionIds;
		Costume missionCommission;
	};

	extern std::unordered_map<uint, Faction> factionById;

	void ReadFactionData();
}