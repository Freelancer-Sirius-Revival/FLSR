#pragma once
#include "Objective.h"
#include "../conditions/CndTrue.h"

namespace Missions
{
	class ObjCndTrue : public CndTrue
	{
	private:
		const ObjectiveParent parent;
		const int objectiveIndex;
		const uint objId;

	public:
		ObjCndTrue(const ObjectiveParent& parent, const int objectiveIndex, const uint objId);
		void ExecuteTrigger();
	};
}