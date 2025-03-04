#pragma once
#include "Action.h"
#include "ActPlaySoundEffectArch.h"

namespace Missions
{
	struct ActPlaySoundEffect : Action
	{
		const ActPlaySoundEffectArchetypePtr archetype;

		ActPlaySoundEffect(Trigger* parentTrigger, const ActPlaySoundEffectArchetypePtr actionArchetype);
		void Execute();
	};
}