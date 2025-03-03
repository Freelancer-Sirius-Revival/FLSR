#pragma once
#include <string>

namespace Missions
{
	struct ActActTriggerArchetype
	{
		std::string triggerName = "";
		bool activate = false;
	};
	typedef std::shared_ptr<ActActTriggerArchetype> ActActTriggerArchetypePtr;
}