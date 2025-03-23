#pragma once
#include <memory>

namespace Missions
{
	struct ActLightFuseArchetype
	{
		unsigned int objNameOrLabel = 0;
		std::string fuseName = "";
	};
	typedef std::shared_ptr<ActLightFuseArchetype> ActLightFuseArchetypePtr;
}