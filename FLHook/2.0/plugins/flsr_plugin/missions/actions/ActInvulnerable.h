#pragma once
#include "Action.h"

namespace Missions
{
	struct ActInvulnerable : Action
	{
		uint objNameOrLabel = 0;
		bool preventNonPlayerDamage = false;
		bool preventPlayerDamage = false;
		float maxHpLossPercentage = 0.0f;

		void Execute(Mission& mission, const MissionObject& activator) const;
	};
	typedef std::shared_ptr<ActInvulnerable> ActInvulnerablePtr;
}