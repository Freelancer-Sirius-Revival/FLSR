#pragma once
#include <iostream>
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
		std::string objNameOrLabel = "";
		Vector position;
		float distance = 0.0f;
		unsigned int systemId = 0;
	};
}