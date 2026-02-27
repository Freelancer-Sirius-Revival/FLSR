#pragma once
#include "Objective.h"
#include "../conditions/CndTimer.h"

namespace Missions
{
	class ObjCndTimer : public CndTimer
	{
	private:
		const ObjectiveParent parent;
		const ObjectiveState state;

	public:
		ObjCndTimer(const ObjectiveParent& parent, const ObjectiveState& state, const float time);
		void ExecuteTrigger();
	};
}