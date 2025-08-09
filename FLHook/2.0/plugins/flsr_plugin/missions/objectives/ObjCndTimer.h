#pragma once
#include "Objective.h"
#include "../conditions/CndTimer.h"

namespace Missions
{
	class ObjCndTimer : public CndTimer
	{
	private:
		const ObjectiveParent parent;
		const int objectiveIndex;
		const uint objId;

	public:
		ObjCndTimer(const ObjectiveParent& parent, const int objectiveIndex, const uint objId, const float time);
		void ExecuteTrigger();
	};
}