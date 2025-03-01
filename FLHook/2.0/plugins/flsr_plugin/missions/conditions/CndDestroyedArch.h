#pragma once
#include <iostream>

namespace Missions
{
	const enum DestroyedCondition
	{
		ALL,
		SILENT,
		EXPLODE
	};

	struct CndDestroyedArchetype
	{
		std::string objNameOrLabel = "";
		int count = 0;
		DestroyedCondition condition = DestroyedCondition::ALL;
		std::string killerNameOrLabel = "";
	};
}