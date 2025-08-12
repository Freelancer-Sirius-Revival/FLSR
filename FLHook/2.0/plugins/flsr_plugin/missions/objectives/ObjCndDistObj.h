#pragma once
#include "Objective.h"
#include "../conditions/CndDistObj.h"

namespace Missions
{
	class ObjCndDistObj : public CndDistObj
	{
	private:
		const ObjectiveParent parent;
		const ObjectiveState state;

	public:
		ObjCndDistObj(const ObjectiveParent& parent, const ObjectiveState& state, const float distance, const uint otherObjNameOrLabel);
		void ExecuteTrigger();
	};
}