#pragma once
#include <string>
#include <memory>

namespace Missions
{
	struct ActDebugMsgArchetype
	{
		std::string message = "";
	};
	typedef std::shared_ptr<ActDebugMsgArchetype> ActDebugMsgArchetypePtr;
}
