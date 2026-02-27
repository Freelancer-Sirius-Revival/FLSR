#pragma once
#include <FLHook.h>

namespace Missions
{
	struct MsnFormation
	{
		uint id;
		Vector position;
		Vector rotation;
		uint formationId;
		std::vector<uint> msnShipIds;
	};
}