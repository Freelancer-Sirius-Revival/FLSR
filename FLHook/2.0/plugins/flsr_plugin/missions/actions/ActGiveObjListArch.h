#pragma once

namespace Missions
{
	struct ActGiveObjListArchetype
	{
		unsigned int objNameOrLabel = 0;
		unsigned int objectivesId = 0;
	};
	typedef std::shared_ptr<ActGiveObjListArchetype> ActGiveObjListArchetypePtr;
}