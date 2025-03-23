#pragma once
#include <memory>

namespace Missions
{
	struct CndBaseEnterArchetype
	{
		unsigned int objNameOrLabel = 0;
		unsigned int baseId = 0;
	};
	typedef std::shared_ptr<CndBaseEnterArchetype> CndBaseEnterArchetypePtr;
}