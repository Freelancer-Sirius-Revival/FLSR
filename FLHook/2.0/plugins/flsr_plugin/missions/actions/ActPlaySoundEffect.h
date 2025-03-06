#pragma once
#include "Action.h"
#include "ActPlaySoundEffectArch.h"

namespace Missions
{
	struct ActPlaySoundEffect : Action
	{
		const ActPlaySoundEffectArchetypePtr archetype;

		ActPlaySoundEffect(const ActionParent& parent, const ActPlaySoundEffectArchetypePtr actionArchetype);
		void Execute();
	};
}