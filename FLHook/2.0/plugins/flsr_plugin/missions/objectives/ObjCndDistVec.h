#pragma once
#include "Objective.h"
#include "../conditions/CndDistVec.h"

namespace Missions
{
	class ObjCndDistVec : public CndDistVec
	{
	private:
		const ObjectiveParent parent;
		const int objectiveIndex;
		const uint objId;

	public:
		ObjCndDistVec(const ObjectiveParent& parent, const int objectiveIndex, const uint objId, const float distance, const Vector& position);
		void ExecuteTrigger();
	};
}