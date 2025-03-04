#pragma once
#include <FLHook.h>

namespace Missions
{
	struct ActSetNNObjArchetype
	{
		uint objNameOrLabel = 0;
		uint message = 0;
		uint systemId = 0;
		Vector position;
		bool bestRoute = false;
		uint targetObjName = 0;
	};
	typedef std::shared_ptr<ActSetNNObjArchetype> ActSetNNObjArchetypePtr;
}