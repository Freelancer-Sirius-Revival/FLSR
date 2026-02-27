#pragma once
#include "Action.h"

namespace Missions
{
	struct ActPlaySoundEffect : Action
	{
		uint label = 0;
		uint soundId = 0;

		void Execute(Mission& mission, const MissionObject& activator) const;
	};
	typedef std::shared_ptr<ActPlaySoundEffect> ActPlaySoundEffectPtr;
}