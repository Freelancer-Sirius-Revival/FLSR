#pragma once

namespace Missions
{
	struct ActAddCargoArchetype
	{
		unsigned int objNameOrLabel = 0;
		unsigned int itemId = 0;
		unsigned int count = 0;
		bool missionFlagged = false;
	};
	typedef std::shared_ptr<ActAddCargoArchetype> ActAddCargoArchetypePtr;
}
