#pragma once
#include <memory>

namespace Missions
{
	struct CndSpaceEnterArchetype
	{
		unsigned int objNameOrLabel = 0;
		unsigned int systemId = 0;
	};
	typedef std::shared_ptr<CndSpaceEnterArchetype> CndSpaceEnterArchetypePtr;
}