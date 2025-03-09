#pragma once
#include <string>

namespace Missions
{
	struct ActSpawnShipArchetype
	{
		std::string msnNpcName = "";
		uint objectivesId = 0;
	};
	typedef std::shared_ptr<ActSpawnShipArchetype> ActSpawnShipArchetypePtr;
}