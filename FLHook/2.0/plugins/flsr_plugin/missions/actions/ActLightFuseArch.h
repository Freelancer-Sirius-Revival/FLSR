#pragma once

namespace Missions
{
	struct ActLightFuseArchetype
	{
		unsigned int objNameOrLabel = 0;
		unsigned int fuseId = 0;
	};
	typedef std::shared_ptr<ActLightFuseArchetype> ActLightFuseArchetypePtr;
}