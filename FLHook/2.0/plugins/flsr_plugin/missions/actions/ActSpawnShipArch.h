#pragma once
#include <string>

namespace Missions
{
	struct ActSpawnShipArchetype
	{
		std::string msnNpcName = "";
	};
	typedef std::shared_ptr<ActSpawnShipArchetype> ActSpawnShipArchetypePtr;
}