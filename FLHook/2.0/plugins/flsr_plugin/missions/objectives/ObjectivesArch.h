#pragma once
#include <iostream>
#include <vector>

namespace Missions
{
	enum class ObjectiveType
	{
		Delay,
		Follow,
		Goto
	};

	typedef std::pair<ObjectiveType, std::shared_ptr<void>> ObjectiveEntry;

	struct ObjectivesArchetype
	{
		std::vector<ObjectiveEntry> objectives;
	};
}