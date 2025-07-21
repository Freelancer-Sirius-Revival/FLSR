#pragma once
#include "Objective.h"
#include "../conditions/CndDistObj.h"

namespace Missions
{
	class ObjCndDistObj : public CndDistObj
	{
	private:
		const ObjectiveParent parent;
		const int objectiveIndex;
		const uint objId;

	public:
		ObjCndDistObj(const ObjectiveParent& parent, const int objectiveIndex, const uint objId, const float distance, const uint otherObjNameOrLabel);
		void ExecuteTrigger();
	};
}