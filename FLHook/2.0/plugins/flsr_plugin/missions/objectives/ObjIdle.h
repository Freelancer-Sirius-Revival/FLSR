#pragma once
#include "Objective.h"

namespace Missions
{
	class ObjIdle : public Objective
	{
	public:
		ObjIdle(const ObjectiveParent& parent);
		void Execute(const ObjectiveState& state) const;
	};
}