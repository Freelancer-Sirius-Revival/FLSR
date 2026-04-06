#pragma once
#include <FLHook.h>

namespace Missions
{
	struct MsnFormation
	{
		uint id = 0;
		Vector position = { 0, 0, 0 };
		Vector rotation = { 0, 0, 0 };
		uint formationId = 0;
		std::vector<uint> msnShipIds;
	};
}