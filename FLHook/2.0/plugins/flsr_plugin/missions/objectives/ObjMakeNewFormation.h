#pragma once
#include <FLHook.h>

namespace Missions
{
	struct ObjMakeNewFormation
	{
		uint formationId;
		std::vector<uint> objNameIds;
	};
	typedef std::shared_ptr<ObjMakeNewFormation> ObjMakeNewFormationPtr;
}