#pragma once
#include "Objective.h"
#include "../conditions/CndTrue.h"

namespace Missions
{
	class ObjCndTrue : public CndTrue
	{
	private:
		const ObjectiveParent parent;
		const ObjectiveState state;

	public:
		ObjCndTrue(const ObjectiveParent& parent, const ObjectiveState& state);
		void ExecuteTrigger();
	};
}