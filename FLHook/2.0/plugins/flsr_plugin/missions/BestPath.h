#pragma once
#include <FLHook.h>

namespace BestPath
{
	void ReadFiles();
	void CollectJumpObjectsPerSystem();
	XRequestBestPathEntry GetJumpObjectToNextSystem(const uint clientId, const uint targetSystem);
}