#pragma once
#include <FLHook.h>

namespace RandomMissions
{
	struct Faction
	{
		uint id;
		std::unordered_set<uint> hostileFactionIds;
		std::unordered_set<uint> npcShipIds;
		std::unordered_set<uint> maleVoiceIds;
		std::unordered_set<uint> femaleVoiceIds;
		Costume missionCommission;
	};

	extern std::unordered_map<uint, Faction> factionById;

	void ReadFactionData();
}