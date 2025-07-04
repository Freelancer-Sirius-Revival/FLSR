#pragma once
#include <FLHook.h>

namespace Missions
{
	struct Npc
	{
		std::string name = "";
		uint archetypeId = 0;
		uint loadoutId = 0;
		std::string stateGraph = "";
		uint pilotId = 0;
		std::string faction = "";
		uint voiceId = 0;
		Costume costume;
		byte level = 0;
	};

	struct MsnNpc
	{
		std::string name = "";
		uint npcId = 0;
		uint idsName = 0;
		uint systemId = 0;
		Vector position;
		Matrix orientation;
		uint pilotJobId = 0;
		uint startingObjId = 0;
		int hitpoints = -1;
		std::unordered_set<uint> labels;
	};
}
