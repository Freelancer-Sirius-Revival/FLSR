#pragma once
#include <memory>
#include <string>

namespace Missions
{
	struct ActSpawnSolarArchetype
	{
		std::string solarName = "";
	};
	typedef std::shared_ptr<ActSpawnSolarArchetype> ActSpawnSolarArchetypePtr;
}