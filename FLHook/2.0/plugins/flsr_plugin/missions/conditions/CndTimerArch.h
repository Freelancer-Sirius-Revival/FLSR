#pragma once
#include <memory>

namespace Missions
{
	struct CndTimerArchetype
	{
		float lowerTimeInS = 0.0f;
		float upperTimeInS = 0.0f;
	};
	typedef std::shared_ptr<CndTimerArchetype> CndTimerArchetypePtr;
}