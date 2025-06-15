#pragma once
#include <memory>

namespace Missions
{
	struct CndCloakedArchetype
	{
		unsigned int objNameOrLabel = 0;
		bool cloaked = false;
	};
	typedef std::shared_ptr<CndCloakedArchetype> CndCloakedArchetypePtr;
}