#pragma once
#include <FLHook.h>

namespace BestPath
{
	struct BestPathNextObject
	{
		Vector position;
		uint objId;
	};

	void ReadFiles();
	BestPathNextObject GetJumpObjectToNextSystem(const uint clientId, const uint targetSystem);
}