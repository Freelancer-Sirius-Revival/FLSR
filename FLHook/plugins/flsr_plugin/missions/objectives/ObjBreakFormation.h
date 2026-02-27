#pragma once
#include "Objective.h"

namespace Missions
{
	class ObjBreakFormation : public Objective
	{
	public:
		ObjBreakFormation(const ObjectiveParent& parent);
		void Execute(const ObjectiveState& state) const;
	};
}