#pragma once
#include "Action.h"
#include "ActPlayMusicArch.h"

namespace Missions
{
	struct ActPlayMusic : Action
	{
		const ActPlayMusicArchetypePtr archetype;

		ActPlayMusic(Trigger* parentTrigger, const ActPlayMusicArchetypePtr actionArchetype);
		void Execute();
	};
}