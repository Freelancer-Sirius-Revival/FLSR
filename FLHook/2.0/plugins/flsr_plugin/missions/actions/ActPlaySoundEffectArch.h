#pragma once

namespace Missions
{
	struct ActPlaySoundEffectArchetype
	{
		unsigned int objNameOrLabel = 0;
		unsigned int soundId = 0;
	};
	typedef std::shared_ptr<ActPlaySoundEffectArchetype> ActPlaySoundEffectArchetypePtr;
}