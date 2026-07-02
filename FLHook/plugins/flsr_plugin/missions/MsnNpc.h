#pragma once
#include <FLHook.h>

namespace Missions
{
	struct MsnNpc
	{
		uint id = 0;
		uint npcShipArchId = 0;
		std::string faction = "";
		uint idsName = 0;
		bool shipNameDisplayed = false;
		uint voiceId = 0;
		Costume costume;
		uint systemId = 0;
		Vector position = { 0, 0, 0 };
		Matrix orientation = EulerMatrix({ 0, 0, 0 });
		uint pilotJobId = 0;
		uint startingObjId = 0;
		int hitpoints = -1;
		std::unordered_set<uint> labels;
	};
}
