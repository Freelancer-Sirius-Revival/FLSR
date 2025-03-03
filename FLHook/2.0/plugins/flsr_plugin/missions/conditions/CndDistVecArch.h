#pragma once
#include <FLHook.h>

namespace Missions
{
	const enum DistanceCondition
	{
		INSIDE,
		OUTSIDE
	};

	struct CndDistVecArchetype
	{
		DistanceCondition type = DistanceCondition::INSIDE;
		unsigned int objNameOrLabel = 0;
		Vector position;
		float distance = 0.0f;
		unsigned int systemId = 0;
	};
	typedef std::shared_ptr<CndDistVecArchetype> CndDistVecArchetypePtr;
}