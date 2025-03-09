#pragma once
#include <iostream>
#include <vector>

namespace Missions
{
	enum class ObjectiveType
	{
		Goto
	};

	typedef std::pair<ObjectiveType, std::shared_ptr<void>> ObjectiveEntry;

	struct ObjectivesArchetype
	{
		std::string name = "";
		std::vector<ObjectiveEntry> objectives;
	};
	typedef std::shared_ptr<ObjectivesArchetype> ObjectivesArchetypePtr;
}