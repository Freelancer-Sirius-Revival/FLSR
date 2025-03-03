#pragma once

namespace Missions
{
	struct ActRemoveLabelArchetype
	{
		unsigned int objNameOrLabel = 0;
		unsigned int label = 0;
	};
	typedef std::shared_ptr<ActRemoveLabelArchetype> ActRemoveLabelArchetypePtr;
}