#pragma once
#include <FLHook.h>

namespace Missions
{
	struct ActDestroyArchetype
	{
		uint objNameOrLabel = 0;
		DestroyType destroyType = DestroyType::VANISH;
	};
	typedef std::shared_ptr<ActDestroyArchetype> ActDestroyArchetypePtr;
}