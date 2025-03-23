#pragma once
#include <memory>
#include <string>

namespace Missions
{
	struct ActActTriggerArchetype
	{
		std::string triggerName = "";
		bool activate = false;
		float probability = 1.0f;
	};
	typedef std::shared_ptr<ActActTriggerArchetype> ActActTriggerArchetypePtr;
}