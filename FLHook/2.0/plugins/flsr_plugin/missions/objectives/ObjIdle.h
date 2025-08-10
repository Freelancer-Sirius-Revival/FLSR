#pragma once
#include "Objective.h"

namespace Missions
{
	class ObjIdle : public Objective
	{
	public:
		ObjIdle(const ObjectiveParent& parent, const int objectiveIndex);
		void Execute(const uint objId) const;
	};
}