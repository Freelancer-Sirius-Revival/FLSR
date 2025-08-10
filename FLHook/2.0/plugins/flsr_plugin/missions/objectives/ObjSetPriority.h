#pragma once
#include "Objective.h"

namespace Missions
{
	class ObjSetPriority : public Objective
	{
	private:
		const bool enforceObjectives;

	public:
		ObjSetPriority(const ObjectiveParent& parent, const bool enforceObjectives);
		void Execute(const ObjectiveState& state) const;
	};
}