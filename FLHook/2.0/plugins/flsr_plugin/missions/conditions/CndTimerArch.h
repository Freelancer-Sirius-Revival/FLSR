#pragma once
#include <memory>

namespace Missions
{
	struct CndTimerArchetype
	{
		float timeInS = 0;
	};
	typedef std::shared_ptr<CndTimerArchetype> CndTimerArchetypePtr;
}