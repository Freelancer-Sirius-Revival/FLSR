#pragma once
#include <string>

namespace Missions
{
	struct ActSpawnShipArchetype
	{
		std::string msnNpcName = "";
		uint objectivesId = 0;
		Vector position = { std::numeric_limits<float>::infinity(), 0, 0 };
		Matrix orientation = { { std::numeric_limits<float>::infinity(), 0, 0 } };
	};
	typedef std::shared_ptr<ActSpawnShipArchetype> ActSpawnShipArchetypePtr;
}