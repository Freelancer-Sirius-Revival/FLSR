#pragma once
#include <memory>

namespace Missions
{
	struct ActGiveObjListArchetype
	{
		unsigned int objNameOrLabel = 0;
		unsigned int objectivesId = 0;
	};
	typedef std::shared_ptr<ActGiveObjListArchetype> ActGiveObjListArchetypePtr;
}