#pragma once
#include "Objective.h"
#include "../conditions/CndDistVec.h"

namespace Missions
{
	class ObjCndDistVec : public CndDistVec
	{
	private:
		const ObjectiveParent parent;
		const ObjectiveState state;

	public:
		ObjCndDistVec(const ObjectiveParent& parent, const ObjectiveState& state, const float distance, const Vector& position);
		void ExecuteTrigger();
	};
}