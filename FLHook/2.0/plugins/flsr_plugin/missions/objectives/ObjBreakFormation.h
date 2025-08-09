#pragma once
#include "Objective.h"

namespace Missions
{
	class ObjBreakFormation : public Objective
	{
	public:
		ObjBreakFormation(const ObjectiveParent& parent, const int objectiveIndex);
		void Execute(const uint objId) const;
	};
}