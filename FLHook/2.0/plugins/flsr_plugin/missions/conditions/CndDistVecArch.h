#pragma once
#include <FLHook.h>

namespace Missions
{
	const enum DistanceCondition
	{
		Inside,
		Outside
	};

	struct CndDistVecArchetype
	{
		DistanceCondition type = DistanceCondition::Inside;
		unsigned int objNameOrLabel = 0;
		Vector position;
		float distance = 0.0f;
		unsigned int systemId = 0;
	};
	typedef std::shared_ptr<CndDistVecArchetype> CndDistVecArchetypePtr;
}