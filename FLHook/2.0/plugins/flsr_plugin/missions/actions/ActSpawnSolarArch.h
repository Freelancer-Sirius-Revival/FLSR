#pragma once
#include <string>

namespace Missions
{
	struct ActSpawnSolarArchetype
	{
		std::string solarName = "";
	};
	typedef std::shared_ptr<ActSpawnSolarArchetype> ActSpawnSolarArchetypePtr;
}