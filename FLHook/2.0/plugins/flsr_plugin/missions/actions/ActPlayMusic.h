#pragma once
#include "Action.h"
#include "ActPlayMusicArch.h"

namespace Missions
{
	struct ActPlayMusic : Action
	{
		const ActPlayMusicArchetypePtr archetype;

		ActPlayMusic(const ActionParent& parent, const ActPlayMusicArchetypePtr actionArchetype);
		void Execute();
	};
}