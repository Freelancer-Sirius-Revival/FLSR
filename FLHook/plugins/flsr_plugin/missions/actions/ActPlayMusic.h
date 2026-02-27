#pragma once
#include "Action.h"

namespace Missions
{
	struct ActPlayMusic : Action
	{
		uint label = 0;
		pub::Audio::Tryptich music;

		void Execute(Mission& mission, const MissionObject& activator) const;
	};
	typedef std::shared_ptr<ActPlayMusic> ActPlayMusicPtr;
}