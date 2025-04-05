#pragma once
#include "Action.h"
#include "../../Empathies.h"

namespace Missions
{
	struct ActAdjRep : Action
	{
		uint objNameOrLabel = 0;
		uint groupId = 0;
		float change = 0.0f;
		Empathies::ReputationChangeReason reason = Empathies::ReputationChangeReason::ObjectDestruction;

		void Execute(Mission& mission, const MissionObject& activator) const;
	};
	typedef std::shared_ptr<ActAdjRep> ActAdjRepPtr;
}