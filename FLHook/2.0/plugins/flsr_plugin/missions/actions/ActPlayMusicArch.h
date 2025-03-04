#pragma once
#include <FLHook.h>

namespace Missions
{
	struct ActPlayMusicArchetype
	{
		unsigned int objNameOrLabel = 0;
		pub::Audio::Tryptich music;
	};
	typedef std::shared_ptr<ActPlayMusicArchetype> ActPlayMusicArchetypePtr;
}