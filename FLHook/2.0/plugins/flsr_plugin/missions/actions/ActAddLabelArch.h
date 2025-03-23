#pragma once
#include <memory>

namespace Missions
{
	struct ActAddLabelArchetype
	{
		unsigned int objNameOrLabel = 0;
		unsigned int label = 0;
	};
	typedef std::shared_ptr<ActAddLabelArchetype> ActAddLabelArchetypePtr;
}