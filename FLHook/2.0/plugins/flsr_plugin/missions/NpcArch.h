#pragma once
#include <FLHook.h>

namespace Missions
{
	struct NpcArchetype
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
	typedef std::shared_ptr<NpcArchetype> NpcArchetypePtr;

	struct MsnNpcArchetype
	{
		std::string name = "";
		uint npcId = 0;
		uint idsName = 0;
		uint systemId = 0;
		Vector position;
		Matrix orientation;
		uint pilotJobId = 0;
		uint startingObjId = 0;
		float hitpoints = -1.0f;
		std::unordered_set<uint> labels;
	};
	typedef std::shared_ptr<MsnNpcArchetype> MsnNpcArchetypePtr;
}
